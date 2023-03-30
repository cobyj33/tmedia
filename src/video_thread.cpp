
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
#include "videoconverter.h"
#include "except.h"

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

const int MAX_FRAME_WIDTH = 27 * 16;
const int MAX_FRAME_HEIGHT = 27 * 9; 
const char* DEBUG_VIDEO_SOURCE = "video";
const char* DEBUG_VIDEO_TYPE = "debug";


void video_playback_thread(MediaPlayer* player, std::mutex& alter_mutex) {
    std::unique_lock<std::mutex> mutex_lock(alter_mutex, std::defer_lock);
    if (!player->has_video()) {
        throw std::runtime_error("Could not playback video data: Could not find video stream in media player");
    }

    StreamData& video_stream_data = player->get_video_stream_data();
    double avg_frame_time_sec = video_stream_data.get_average_frame_time_sec();
    AVCodecContext* video_codec_context = video_stream_data.get_codec_context();

    std::pair<int, int> bounded_video_frame_dimensions = get_bounded_dimensions(video_codec_context->width, video_codec_context->height, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
    int output_frame_width = bounded_video_frame_dimensions.first;
    int output_frame_height = bounded_video_frame_dimensions.second;

    VideoConverter videoConverter(output_frame_width, output_frame_height, AV_PIX_FMT_RGB24, video_codec_context->width, video_codec_context->height, video_codec_context->pix_fmt);

    while (player->in_use) {
        if (!player->playback.is_playing()) {
            sleep_quick();
            continue;
        }
        mutex_lock.lock();

        double frame_duration = avg_frame_time_sec;
        double frame_pts_time_sec = player->get_time(system_clock_sec()) + frame_duration;
        double extra_delay = 0.0;

        try {
            std::vector<AVFrame*> decoded_frames = player->next_video_frames();

            if (decoded_frames.size() > 0) {
                AVFrame* frame_image = videoConverter.convert_video_frame(decoded_frames[0]);
                frame_duration = (double)frame_image->duration * video_stream_data.get_time_base();
                frame_pts_time_sec = (double)frame_image->pts * video_stream_data.get_time_base();
                extra_delay = (double)(frame_image->repeat_pict) / (2 * avg_frame_time_sec);
                player->set_current_frame(frame_image);
                av_frame_free(&frame_image);
            }

            clear_av_frame_list(decoded_frames);
        } catch (std::exception const& e) {
            PixelData error_image = VideoIcon::ERROR_ICON.pixel_data;
            player->set_current_frame(error_image);
        }

        double frame_speed_skip_time_sec = ( frame_duration - ( frame_duration / player->playback.get_speed() ) );
        player->playback.skip(frame_speed_skip_time_sec);

        const double current_time = player->get_time(system_clock_sec());
        double waitDuration = frame_pts_time_sec - current_time + extra_delay - frame_speed_skip_time_sec;

        mutex_lock.unlock();
        if (waitDuration <= 0) {
            continue;
        } else {
            sleep_for_sec(waitDuration);
        }
    }

}



