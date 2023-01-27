
#include "boiler.h"
#include "decode.h"
#include "threads.h"
#include "playheadlist.hpp"
#include "icons.h"
#include "renderer.h"
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <video.h>
#include <media.h>
#include <ascii.h>
#include <wmath.h>
#include <wtime.h>
#include <videoconverter.h>
#include <mutex>

#include <stdexcept>

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

const char* debug_video_source = "video";
const char* debug_video_type = "debug";

// void load_image_buffer(MediaPlayer* player, VideoConverter* converter, int amount) {
//     const MediaDisplayCache* cache = player->displayCache;
//     for (int i = 0; i < amount; i++) {

//     }
// }

void* video_playback_thread(MediaPlayer* player, std::mutex& alter_mutex) {

    AVFrame* readingFrame = av_frame_alloc();
    std::unique_lock<std::mutex> mutex_lock(alter_mutex, std::defer_lock);

    Playback* playback = player->timeline->playback;
    MediaDebugInfo* debug_info = player->displayCache->debug_info;
    MediaData* media_data = player->timeline->mediaData;
    MediaDisplayCache* cache = player->displayCache;
    MediaStream* video_stream = get_media_stream(media_data, AVMEDIA_TYPE_VIDEO);
    if (video_stream == NULL) {
        throw std::runtime_error("COULD NOT FIND VIDEO STREAM");
    }

    double frameRate = av_q2d(video_stream->info->stream->avg_frame_rate);
    double videoTimeBase = video_stream->timeBase;

    AVCodecContext* videoCodecContext = video_stream->info->codecContext;
    int output_frame_width, output_frame_height;
    get_output_size(videoCodecContext->width, videoCodecContext->height, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, &output_frame_width, &output_frame_height);

    const int use_colors = player->displaySettings->use_colors;
    VideoConverter* videoConverter = new VideoConverter(output_frame_width, output_frame_height, use_colors == 1 ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_GRAY8, videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt);

    PlayheadList<AVPacket*>* videoPackets = video_stream->packets;
    while (player->inUse && (!media_data->allPacketsRead || (media_data->allPacketsRead && videoPackets->can_step_forward() ) )) {
        mutex_lock.lock();
        /* int64_t targetVideoPTS = get_playback_current_time(playback) * videoTimeBase; */
        /* move_frame_list_to_pts(cache->image_buffer, targetVideoPTS); */

        if (playback->get_time(system_clock_sec()) >= media_data->duration) { // video finished
            player->inUse = false;
            mutex_lock.unlock();
            break;
        } else if (playback->is_playing() == false) { // paused
            mutex_lock.unlock();

            while (playback->is_playing() == false) {
                sleep_for_ms(1);
                if (!player->inUse) {
                    return nullptr;
                }
            }

            mutex_lock.lock();
            // playback->paused_time += system_clock_sec() - pauseTime;
        }

        if (videoPackets->can_move_index(10)) {
            int decodeResult;
            int nb_decoded;
            AVPacket* currentPacket = videoPackets->get();
            while (currentPacket == NULL && videoPackets->can_step_forward()) {
                videoPackets->step_forward();
                currentPacket = videoPackets->get();
            }
            if (currentPacket == NULL) {
                mutex_lock.unlock();
                continue;
            }

            AVFrame** decodedList = decode_video_packet(videoCodecContext, currentPacket, &decodeResult, &nb_decoded);

            int repeats = 1;
            while (decodeResult == AVERROR(EAGAIN) && videoPackets->can_step_forward()) {
                repeats++;
                videoPackets->step_forward();
                free_frame_list(decodedList, nb_decoded);
                currentPacket = videoPackets->get();
                while (currentPacket == NULL && videoPackets->can_step_forward()) {
                    videoPackets->step_forward();
                    currentPacket = videoPackets->get();
                }
                if (currentPacket == NULL) {
                    break;
                }

                decodedList = decode_video_packet(videoCodecContext, currentPacket, &decodeResult, &nb_decoded);
            }

            add_debug_message(debug_info, debug_video_source, debug_video_type, "Number of video packets sent to decoder","Fed %d packets to decode\n", repeats);

            if ((nb_decoded > 0 && decodeResult >= 0) || currentPacket == NULL) {
                av_frame_free(&readingFrame);
                add_debug_message(debug_info, debug_video_source, debug_video_type, "Time of decoded video packet","Decoded List Time: %.3f", decodedList[0]->pts * videoTimeBase );
                readingFrame = videoConverter->convert_video_frame(decodedList[0]);
                free_frame_list(decodedList, nb_decoded);
            } else {
                add_debug_message(debug_info, debug_video_source, debug_video_type, "Null video packet", "ERROR: NULL POINTED VIDEO PACKET: %d", decodeResult);
                videoPackets->try_step_forward();

                mutex_lock.unlock();
                fsleep_for_sec(1.0 / frameRate);
                continue;
            }

            videoPackets->try_step_forward();
        } else {
            while (!media_data->allPacketsRead && !videoPackets->can_move_index(20)) {
                fetch_next(media_data, 20);
            }
            mutex_lock.unlock();
            continue;
        }

        if (player->displayCache->image != NULL) {
            pixel_data_free(player->displayCache->image);
        }
        player->displayCache->image = pixel_data_alloc_from_frame(readingFrame);

        double nextFrameTimeSinceStartInSeconds = (double)readingFrame->pts * videoTimeBase;
        double frame_speed_skip_time_sec = ( (readingFrame->duration * videoTimeBase) - (readingFrame->duration * videoTimeBase) / playback->get_speed() );
        playback->skip(frame_speed_skip_time_sec);

        const double current_time = playback->get_time(system_clock_sec());
        double waitDuration = nextFrameTimeSinceStartInSeconds - current_time + (double)(readingFrame->repeat_pict) / (2 * frameRate);
        waitDuration -= frame_speed_skip_time_sec;
        double continueTime = system_clock_sec() + waitDuration;

        add_debug_message(debug_info, debug_video_source, debug_video_type, "Video Timing Information", "  timeOfNextFrame: %.3f, waitDuration: %.3f\n \
            Speed Factor: %.3f, Time Skipped due to Speed on Current Frame: %.3f\n\n ",
           nextFrameTimeSinceStartInSeconds, waitDuration,
            playback->get_speed(), frame_speed_skip_time_sec);


        mutex_lock.unlock();
        av_frame_unref(readingFrame);
        if (waitDuration <= 0) {
            continue;
        }
        /* fsleep_for_sec(waitDuration); */
        while (system_clock_sec() < continueTime) {

        }
    }

    av_frame_free(&readingFrame);
    return NULL;
}

void jump_to_time(MediaTimeline* timeline, double targetTime) {
    targetTime = fmax(targetTime, 0.0);
    Playback* playback = timeline->playback;
    MediaData* media_data = timeline->mediaData;
    const double originalTime = timeline->playback->get_time(system_clock_sec());
    MediaStream* video_stream = get_media_stream(media_data, AVMEDIA_TYPE_VIDEO);
    if (video_stream == NULL) {
        return;
    }

    AVCodecContext* videoCodecContext = video_stream->info->codecContext;
    PlayheadList<AVPacket*>* videoPackets = video_stream->packets;
    double videoTimeBase = video_stream->timeBase;
    const int64_t targetVideoPTS = targetTime / videoTimeBase;
    AVPacket* packet_get;

    if (targetTime == originalTime) {
        return;
     } else if (targetTime < originalTime || targetTime > originalTime + 60) {
        avcodec_flush_buffers(videoCodecContext);
        double testTime = fmax(0.0, targetTime - 30);
        packet_get = videoPackets->get();
        if (packet_get == NULL) { return; }
        double last_time = packet_get->pts * videoTimeBase;
        if (packet_get != NULL) {
            while (videoPackets->can_move_index(signum(testTime - originalTime))) {
                videoPackets->move_index(signum(testTime - originalTime));
                packet_get = videoPackets->get();
                if (packet_get == NULL) {
                    break;
                }

                if (testTime >= fmin(last_time, packet_get->pts * videoTimeBase) && testTime <= fmax(last_time, packet_get->pts * videoTimeBase)) {
                    break;
                }
                last_time = packet_get->pts * videoTimeBase;
            }
        }
     }

    int64_t finalPTS = -1;
    int readingStatus = 0;
    int nb_decoded = 0;

    /* while (finalPTS < targetVideoPTS && videoPackets->get_index() + 1 < videoPackets->get_length() && (readingStatus > 0 || readingStatus == AVERROR(EAGAIN))  ) { */
    while (1) {
        if (finalPTS >= targetVideoPTS || !videoPackets->can_step_forward() || (readingStatus < 0 && readingStatus != AVERROR(EAGAIN)) ) {
            break;
        } 

        packet_get = videoPackets->get();
        if (packet_get == NULL) {
            videoPackets->step_forward();
            continue;
        }

        AVFrame** decodedFrames = decode_video_packet(videoCodecContext, packet_get, &readingStatus, &nb_decoded);
        videoPackets->step_forward();

        while (readingStatus == AVERROR(EAGAIN) && videoPackets->can_step_forward() && nb_decoded > 0 && decodedFrames != NULL) {
            free_frame_list(decodedFrames, nb_decoded);
            packet_get = videoPackets->get();
            if (packet_get == NULL) {
                continue;
            }

            decodedFrames = decode_video_packet(videoCodecContext, packet_get, &readingStatus, &nb_decoded);
            videoPackets->step_forward();
        }

        if (readingStatus >= 0) {
            finalPTS = decodedFrames[0]->pts;
        }
        if (decodedFrames != NULL) {
            free_frame_list(decodedFrames, nb_decoded);
        }
    }

    packet_get = videoPackets->get();
    finalPTS = finalPTS == -1 && packet_get != NULL ? packet_get->pts : finalPTS;
    double timeMoved = (finalPTS * videoTimeBase) - originalTime;
    playback->skip(timeMoved);
}

