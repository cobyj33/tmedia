
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <iostream>

#include "media.h"
#include "boiler.h"
#include "decode.h"
#include "threads.h"
#include "icons.h"
#include "ascii.h"
#include "wmath.h"
#include "wtime.h"
#include "playheadlist.hpp"
#include "videoconverter.h"
#include "except.h"

extern "C" {
#include <curses.h>
#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

const int MAX_FRAME_WIDTH = 16 * 16;
const int MAX_FRAME_HEIGHT = 16 * 9; 
const char* DEBUG_VIDEO_SOURCE = "video";
const char* DEBUG_VIDEO_TYPE = "debug";


void video_playback_thread(MediaPlayer* player, std::mutex& alter_mutex) {
    std::unique_lock<std::mutex> mutex_lock(alter_mutex, std::defer_lock);

    Playback& playback = player->timeline->playback;
    std::unique_ptr<MediaData>& media_data = player->timeline->mediaData;
    MediaDisplayCache& cache = player->displayCache;

    if (!media_data->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        throw std::runtime_error("Could not playback video data: Could not find video stream in media player");
    }

    MediaStream& video_stream = media_data->get_media_stream(AVMEDIA_TYPE_VIDEO);
    double frame_time_sec = video_stream.get_average_frame_time_sec();
    AVCodecContext* videoCodecContext = video_stream.get_codec_context();

    std::pair<int, int> bounded_video_frame_dimensions = get_bounded_dimensions(videoCodecContext->width, videoCodecContext->height, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
    int output_frame_width = bounded_video_frame_dimensions.first;
    int output_frame_height = bounded_video_frame_dimensions.second;

    VideoConverter videoConverter(output_frame_width, output_frame_height, AV_PIX_FMT_RGB24, videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt);

    while (player->inUse && playback.get_time(system_clock_sec()) < media_data->duration) {
        if (!playback.is_playing() || !video_stream.packets.can_move_index(10)) { // paused or not enough data
            sleep_quick();
            continue;
        }
        mutex_lock.lock();
        double frame_duration = frame_time_sec;
        double frame_pts_time_sec = playback.get_time(system_clock_sec()) + frame_duration;
        double extra_delay = 0.0;

        try {
            std::vector<AVFrame*> decodedList = decode_video_packet(videoCodecContext, video_stream.packets);

            if (decodedList.size() > 0) {
                AVFrame* frame_image = videoConverter.convert_video_frame(decodedList[0]);
                frame_duration = (double)frame_image->duration * video_stream.get_time_base();
                frame_pts_time_sec = (double)frame_image->pts * video_stream.get_time_base();
                extra_delay = (double)(frame_image->repeat_pict) / (2 * frame_time_sec);
                PixelData pixel_image(frame_image);
                player->set_current_image(pixel_image);
                av_frame_free(&frame_image);
            }

            clear_av_frame_list(decodedList);
        } catch (std::exception e) {
            PixelData error_image = VideoIcon::ERROR_ICON.pixelData;
            player->set_current_image(error_image);
        }

        double frame_speed_skip_time_sec = ( frame_duration - ( frame_duration / playback.get_speed() ) );
        playback.skip(frame_speed_skip_time_sec);

        const double current_time = playback.get_time(system_clock_sec());
        double waitDuration = frame_pts_time_sec - current_time + extra_delay - frame_speed_skip_time_sec;

        video_stream.packets.try_step_forward();
        mutex_lock.unlock();
        if (waitDuration <= 0) {
            continue;
        } else {
            sleep_for_sec(waitDuration);
        }
    }

}



