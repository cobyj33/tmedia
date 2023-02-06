
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <memory>
#include <stdexcept>

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
    AVFrame* readingFrame = av_frame_alloc();

    Playback& playback = player->timeline->playback;
    // MediaDebugInfo& debug_info = player->displayCache.debug_info;
    std::unique_ptr<MediaData>& media_data = player->timeline->mediaData;
    MediaDisplayCache& cache = player->displayCache;

    if (!media_data->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        throw std::runtime_error("Could not playback video data: Could not find video stream in media player");
    }

    MediaStream& video_stream = media_data->get_media_stream(AVMEDIA_TYPE_VIDEO);

    double frameRate = video_stream.get_average_frame_rate();
    double videoTimeBase = video_stream.get_time_base();
    AVCodecContext* videoCodecContext = video_stream.get_codec_context();

    std::pair<int, int> bounded_video_frame_dimensions = get_bounded_dimensions(videoCodecContext->width, videoCodecContext->height, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
    int output_frame_width = bounded_video_frame_dimensions.first;
    int output_frame_height = bounded_video_frame_dimensions.second;

    const bool use_colors = player->displaySettings.use_colors;
    VideoConverter videoConverter(output_frame_width, output_frame_height, AV_PIX_FMT_RGB24, videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt);

    PlayheadList<AVPacket*>& videoPackets = video_stream.packets;
    while (player->inUse && (!media_data->allPacketsRead || (media_data->allPacketsRead && videoPackets.can_step_forward() ) )) {
        mutex_lock.lock();

        if (playback.get_time(system_clock_sec()) >= media_data->duration) { // video finished
            player->inUse = false;
            mutex_lock.unlock();
            break;
        } else if (playback.is_playing() == false || !videoPackets.can_move_index(10)) { // paused
            mutex_lock.unlock();
            sleep_for_ms(1);
            continue;
        }

        std::vector<AVFrame*> decodedList;
        while (decodedList.size() == 0) {
            try {
                std::vector<AVFrame*> decodedList = decode_video_packet(videoCodecContext, videoPackets.get());
            } catch (ascii::ffmpeg_error e) {
                if (e.is_eagain() && videoPackets.can_step_forward()) {
                    clear_av_frame_list(decodedList);
                    videoPackets.step_forward();
                } else {
                    break;
                }
            }
        }

        if (decodedList.size() > 0) {
            av_frame_free(&readingFrame);
            readingFrame = videoConverter.convert_video_frame(decodedList[0]);
            clear_av_frame_list(decodedList);
        } else {
            videoPackets.try_step_forward();
            mutex_lock.unlock();
            sleep_for_sec(1.0 / frameRate);
            continue;
        }

        videoPackets.try_step_forward();

        std::vector<std::vector<uint8_t>> test{
            { 255, 255, 120, 120, 255, 255 },
            { 255, 255, 120, 120, 255, 255 },
            { 255, 255, 120, 120, 255, 255 },
            { 255, 255, 120, 120, 255, 255 },
            { 255, 255, 120, 120, 255, 255 },
            { 255, 255, 120, 120, 255, 255 },
            { 255, 255, 120, 120, 255, 255 },
            { 255, 255, 120, 120, 255, 255 }
        };
        PixelData image(test);
        player->set_current_image(image);
        sleep_for_sec(frameRate);

        // double next_frame_time_since_start_sec = (double)readingFrame->pts * videoTimeBase;
        // double frame_speed_skip_time_sec = ( (readingFrame->duration * videoTimeBase) - ( (readingFrame->duration * videoTimeBase) / playback.get_speed() ) );
        // playback.skip(frame_speed_skip_time_sec);

        // const double current_time = playback.get_time(system_clock_sec());
        // double extra_delay = (double)(readingFrame->repeat_pict) / (2 * frameRate);
        // double waitDuration = next_frame_time_since_start_sec - current_time + extra_delay - frame_speed_skip_time_sec;
        // waitDuration -= frame_speed_skip_time_sec;
        // mutex_lock.unlock();

        // av_frame_unref(readingFrame);
        // if (waitDuration <= 0) {
        //     continue;
        // } else {
        //     sleep_for_sec(waitDuration);
        // }
    }

    av_frame_free(&readingFrame);
}



