#include "boiler.h"
#include "decode.h"
#include "doublelinkedlist.hpp"
#include "icons.h"
#include "renderer.h"
#include <cstdint>
#include <libavutil/pixfmt.h>
#include <video.h>
#include <macros.h>
#include <media.h>
#include <ascii.h>
#include <loader.h>

#include <thread>
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

double nanoseconds_duration_to_seconds(std::chrono::nanoseconds nanoseconds) {
    return (double)nanoseconds.count() / SECONDS_TO_NANOSECONDS;
}

void video_playback_thread(MediaPlayer* player, std::mutex* alterMutex) {
    std::unique_lock<std::mutex> alterLock{*alterMutex, std::defer_lock};

    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    /* std::chrono::nanoseconds speed_skipped_time = std::chrono::nanoseconds(0); */

    AVFrame* readingFrame = av_frame_alloc();
    Playback* playback = player->timeline->playback;
    MediaDebugInfo* debug_info = player->displayCache->debug_info;
    MediaData* media_data = player->timeline->mediaData;
    MediaStream* video_stream = get_media_stream(media_data, AVMEDIA_TYPE_VIDEO);
    MediaClock* clock = player->timeline->playback->clock;
    if (video_stream == nullptr) {
        return;
    }

    double frameRate = av_q2d(video_stream->info->stream->avg_frame_rate);
    double videoTimeBase = video_stream->timeBase;

    AVCodecContext* videoCodecContext = video_stream->info->codecContext;
    int output_frame_width, output_frame_height;
    get_output_size(videoCodecContext->width, videoCodecContext->height, MAX_ASCII_IMAGE_WIDTH, MAX_ASCII_IMAGE_HEIGHT, &output_frame_width, &output_frame_height);

    const bool use_colors = player->displaySettings->use_colors;

    VideoConverter* videoConverter = get_video_converter(output_frame_width, output_frame_height, use_colors ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_GRAY8, videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt);
    
    if (videoConverter == nullptr) {
        return;
    }

    DoubleLinkedList<AVPacket*>* videoPackets = video_stream->packets;
    int64_t counter = 0;

    while (player->inUse && (!media_data->allPacketsRead || (media_data->allPacketsRead && videoPackets->get_index() < videoPackets->get_length() - 1 ) )) {
        alterLock.lock();
        counter++;

        if (playback->time + 0.5 >= media_data->duration) {
            player->inUse = false;
            alterLock.unlock();
            break;
        }

        if (playback->playing == false) {
            alterLock.unlock();
            std::chrono::steady_clock::time_point pauseTime = std::chrono::steady_clock::now();
            while (playback->playing == false) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                if (!player->inUse) {
                    return;
                }
            }
            alterLock.lock();
            clock->realTimePaused += std::chrono::steady_clock::now() - pauseTime;
            video_symbol_stack_push(player->displayCache->symbol_stack, get_video_symbol(PLAY_ICON));
        }

        if (videoPackets->get_index() + 10 <= videoPackets->get_length()) {
            int decodeResult;
            int nb_decoded;
            AVFrame** decodedList = decode_video_packet(videoCodecContext, videoPackets->get(), &decodeResult, &nb_decoded);

            int repeats = 1;
            while (decodeResult == AVERROR(EAGAIN) && videoPackets->get_index() + 1 < videoPackets->get_length()) {
                repeats++;
                videoPackets->set_index(videoPackets->get_index() + 1);
                free_frame_list(decodedList, nb_decoded);
                decodedList = decode_video_packet(videoCodecContext, videoPackets->get(), &decodeResult, &nb_decoded);
            }

            add_debug_message(debug_info, "Fed %d packets to decode\n", repeats);

            if (nb_decoded > 0 && decodeResult >= 0) {
                av_frame_free(&readingFrame);
                add_debug_message(debug_info, "Decoded List Time: %f", decodedList[0]->pts * videoTimeBase );
                readingFrame = convert_video_frame(videoConverter, decodedList[0]);
                free_frame_list(decodedList, nb_decoded);
            } else {
                add_debug_message(debug_info, "ERROR: NULL POINTED VIDEO FRAME: ERROR: %s");
                videoPackets->try_move_index(1);

                alterLock.unlock();
                std::this_thread::sleep_for(std::chrono::nanoseconds((int64_t)(1 / frameRate * SECONDS_TO_NANOSECONDS )));
                continue;
            }

            videoPackets->try_move_index(1);
        } else {
            while (!media_data->allPacketsRead && videoPackets->can_move_index(10)) {
                fetch_next(media_data, 10);
            }
            alterLock.unlock();
            continue;
        }

        if (player->displayCache->image != nullptr) {
            pixel_data_free(player->displayCache->image);
        }

        player->displayCache->image = pixel_data_alloc_from_frame(readingFrame);

        double nextFrameTimeSinceStartInSeconds = (double)readingFrame->pts * videoTimeBase;
        playback->time = nextFrameTimeSinceStartInSeconds; 

        std::chrono::nanoseconds frame_speed_skip_time = std::chrono::nanoseconds( (int64_t) ( ( (readingFrame->duration * videoTimeBase) - (readingFrame->duration * videoTimeBase) / playback->speed ) * SECONDS_TO_NANOSECONDS ) ); ;
        clock->realTimeSkipped += frame_speed_skip_time;


        //TODO: Check if adding speed_skipped_time actually does anything (since nextFrameTimeSinceStartInSeconds is altered by state->playback->speed)
        clock->realTimeElapsed = std::chrono::steady_clock::now() - start_time - clock->realTimePaused + clock->realTimeSkipped;
        std::chrono::nanoseconds timeOfNextFrame = std::chrono::nanoseconds((int64_t)(nextFrameTimeSinceStartInSeconds * SECONDS_TO_NANOSECONDS));
        std::chrono::nanoseconds waitDuration = timeOfNextFrame - clock->realTimeElapsed + std::chrono::nanoseconds( (int)((double)(readingFrame->repeat_pict) / (2 * frameRate) * SECONDS_TO_NANOSECONDS) );
        waitDuration -= frame_speed_skip_time;
        std::chrono::time_point<std::chrono::steady_clock> continueTime = std::chrono::steady_clock::now() + waitDuration;

        add_debug_message(debug_info, "master time: %f, time paused: %f\n \
            time Elapsed: %f,  timeOfNextFrame: %f, waitDuration: %f\n \
            Speed Factor: %f, Time Skipped: %f, Time Skipped due to Speed on Current Frame: %f\n\n ",
           playback->time, nanoseconds_duration_to_seconds(clock->realTimePaused),
           nanoseconds_duration_to_seconds(clock->realTimeElapsed), nanoseconds_duration_to_seconds(timeOfNextFrame), nanoseconds_duration_to_seconds(waitDuration),
            playback->speed, nanoseconds_duration_to_seconds(clock->realTimeSkipped), nanoseconds_duration_to_seconds(frame_speed_skip_time));


        alterLock.unlock();
        av_frame_unref(readingFrame);
        if (waitDuration <= std::chrono::nanoseconds(0)) {
            continue;
        }

        std::chrono::time_point<std::chrono::steady_clock> lastWaitTime = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() < continueTime) {
            alterLock.lock();
            playback->time += (double)( (double)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - lastWaitTime).count() / SECONDS_TO_NANOSECONDS );
            alterLock.unlock();
            lastWaitTime = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

    }

    av_frame_free(&readingFrame);
}

void jump_to_time(MediaTimeline* timeline, double targetTime) {
    Playback* playback = timeline->playback;
    MediaData* media_data = timeline->mediaData;
    const double originalTime = playback->time;
    MediaStream* video_stream = get_media_stream(media_data, AVMEDIA_TYPE_VIDEO);
    if (video_stream == nullptr) {
        return;
    }

    AVCodecContext* videoCodecContext = video_stream->info->codecContext;
    DoubleLinkedList<AVPacket*>* videoPackets = video_stream->packets;
    double videoTimeBase = video_stream->timeBase;
    const int64_t targetVideoPTS = targetTime / videoTimeBase;

    if (targetTime == playback->time) {
        return;
     } else if (targetTime < playback->time) {
        avcodec_flush_buffers(videoCodecContext);
        double testTime = std::max(0.0, targetTime - 30);
        while (videoPackets->get()->pts * videoTimeBase > testTime && videoPackets->can_move_index(-1)) {
            videoPackets->try_move_index(-1);
        }
    }

    int64_t finalPTS = -1;
    int readingStatus = 0;
    int nb_decoded = 0;

    /* while (finalPTS < targetVideoPTS && videoPackets->get_index() + 1 < videoPackets->get_length() && (readingStatus > 0 || readingStatus == AVERROR(EAGAIN))  ) { */
    while (true) {
        if (finalPTS >= targetVideoPTS || (videoPackets->get_index() + 1 >= videoPackets->get_length()) || (readingStatus < 0 && readingStatus != AVERROR(EAGAIN)) ) {
            break;
        } 

        AVFrame** decodedFrames = decode_video_packet(videoCodecContext, videoPackets->get(), &readingStatus, &nb_decoded);
        videoPackets->try_move_index(1);

        while (readingStatus == AVERROR(EAGAIN) && videoPackets->can_move_index(1) && nb_decoded > 0 && decodedFrames != nullptr) {
            free_frame_list(decodedFrames, nb_decoded);
            decodedFrames = decode_video_packet(videoCodecContext, videoPackets->get(), &readingStatus, &nb_decoded);
            videoPackets->try_move_index(1);
        }

        if (readingStatus >= 0) {
            finalPTS = decodedFrames[0]->pts;
        }
        if (decodedFrames != nullptr) {
            free_frame_list(decodedFrames, nb_decoded);
        }
    }

    finalPTS = finalPTS == -1 ? videoPackets->get()->pts : finalPTS;
    double timeMoved = (finalPTS * videoTimeBase) - originalTime;
    playback->clock->realTimeSkipped += std::chrono::nanoseconds((int64_t)( timeMoved * SECONDS_TO_NANOSECONDS ));
    playback->time = finalPTS * videoTimeBase;

    
    for (int i = 0; i < timeline->mediaData->nb_streams; i++) {
        timeline->mediaData->media_streams[i]->streamTime = playback->time;
    }
}

