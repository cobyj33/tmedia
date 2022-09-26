#include "boiler.h"
#include "decode.h"
#include "threads.h"
#include "selectionlist.h"
#include "icons.h"
#include "renderer.h"
#include <curses.h>
#include <stdint.h>
#include <libavutil/pixfmt.h>
#include <stdio.h>
#include <time.h>
#include <video.h>
#include <macros.h>
#include <media.h>
#include <ascii.h>
#include <loader.h>
#include <wmath.h>

#include <pthread.h>
#include <wtime.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

const int MAX_FRAME_WIDTH = 16 * 16;
const int MAX_FRAME_HEIGHT = 16 * 9; 

const char* debug_video_source = "video";
const char* debug_video_type = "debug";

void* video_playback_thread(void* args) {
    MediaThreadData* thread_data = (MediaThreadData*)args;
    MediaPlayer* player = thread_data->player;
    pthread_mutex_t* alterMutex = thread_data->alterMutex;

    AVFrame* readingFrame = av_frame_alloc();
    Playback* playback = player->timeline->playback;
    MediaDebugInfo* debug_info = player->displayCache->debug_info;
    MediaData* media_data = player->timeline->mediaData;
    MediaStream* video_stream = get_media_stream(media_data, AVMEDIA_TYPE_VIDEO);
    if (video_stream == NULL) {
        fprintf(stderr, "%s\n", "COULD NOT FIND VIDEO STREAM");
        return NULL;
    }

    double frameRate = av_q2d(video_stream->info->stream->avg_frame_rate);
    double videoTimeBase = video_stream->timeBase;

    AVCodecContext* videoCodecContext = video_stream->info->codecContext;
    int output_frame_width, output_frame_height;
    get_output_size(videoCodecContext->width, videoCodecContext->height, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, &output_frame_width, &output_frame_height);

    const int use_colors = player->displaySettings->use_colors;
    VideoConverter* videoConverter = get_video_converter(output_frame_width, output_frame_height, use_colors == 1 ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_GRAY8, videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt);

    if (videoConverter == NULL) {
        fprintf(stderr, "%s\n", "COULD NOT ALLOCATE VIDEO CONVERTER");
        return NULL;
    }

    SelectionList* videoPackets = video_stream->packets;
    int64_t counter = 0;

    while (player->inUse && (!media_data->allPacketsRead || (media_data->allPacketsRead && selection_list_can_move_index(videoPackets, 1) ) )) {
        pthread_mutex_lock(alterMutex);
        counter++;

        if (get_playback_current_time(playback) >= media_data->duration) {
            player->inUse = 0;
            pthread_mutex_unlock(alterMutex);
            break;
        } else if (playback->playing == 0) {
            pthread_mutex_unlock(alterMutex);
            double pauseTime = clock_sec();
            while (playback->playing == 0) {
                sleep_for_ms(1);
                if (!player->inUse) {
                    return NULL;
                }
            }

            pthread_mutex_lock(alterMutex);
            playback->paused_time += clock_sec() - pauseTime;
            video_symbol_stack_push(player->displayCache->symbol_stack, get_video_symbol(PLAY_ICON));
        }

        if (selection_list_can_move_index(videoPackets, 10)) {
            int decodeResult;
            int nb_decoded;
            AVPacket* currentPacket = (AVPacket*)selection_list_get(videoPackets);
            while (currentPacket == NULL && selection_list_can_move_index(videoPackets, 1)) {
                selection_list_try_move_index(videoPackets, 1);
                currentPacket = (AVPacket*)selection_list_get(videoPackets);
            }
            if (currentPacket == NULL) {
                pthread_mutex_unlock(alterMutex);
                continue;
            }

            AVFrame** decodedList = decode_video_packet(videoCodecContext, currentPacket, &decodeResult, &nb_decoded);

            int repeats = 1;
            while (decodeResult == AVERROR(EAGAIN) && selection_list_can_move_index(videoPackets, 1)) {
                repeats++;
                selection_list_try_move_index(videoPackets, 1);
                free_frame_list(decodedList, nb_decoded);
                currentPacket = (AVPacket*)selection_list_get(videoPackets);
                while (currentPacket == NULL && selection_list_can_move_index(videoPackets, 1)) {
                    selection_list_try_move_index(videoPackets, 1);
                    currentPacket = (AVPacket*)selection_list_get(videoPackets);
                }
                if (currentPacket == NULL) {
                    break;
                }

                decodedList = decode_video_packet(videoCodecContext, currentPacket, &decodeResult, &nb_decoded);
            }

            add_debug_message(debug_info, debug_video_source, debug_video_type, "Fed %d packets to decode\n", repeats);

            if ((nb_decoded > 0 && decodeResult >= 0) || currentPacket == NULL) {
                av_frame_free(&readingFrame);
                add_debug_message(debug_info, debug_video_source, debug_video_type, "Decoded List Time: %f", decodedList[0]->pts * videoTimeBase );
                readingFrame = convert_video_frame(videoConverter, decodedList[0]);
                free_frame_list(decodedList, nb_decoded);
            } else {
                add_debug_message(debug_info, debug_video_source, debug_video_type, "ERROR: NULL POINTED VIDEO FRAME: %d", decodeResult);
                printw("ERROR: NULL POINTED VIDEO FRAME: ERROR: %d", decodeResult);
                selection_list_try_move_index(videoPackets, 1);

                pthread_mutex_unlock(alterMutex);
                fsleep_for_sec(1.0 / frameRate);
                continue;
            }

            selection_list_try_move_index(videoPackets, 1);
        } else {
            while (!media_data->allPacketsRead && !selection_list_can_move_index(videoPackets, 20)) {
                fetch_next(media_data, 20);
            }
            pthread_mutex_unlock(alterMutex);
            continue;
        }

        if (player->displayCache->image != NULL) {
            pixel_data_free(player->displayCache->image);
        }
        player->displayCache->image = pixel_data_alloc_from_frame(readingFrame);

        double nextFrameTimeSinceStartInSeconds = (double)readingFrame->pts * videoTimeBase;
        double frame_speed_skip_time_sec = ( (readingFrame->duration * videoTimeBase) - (readingFrame->duration * videoTimeBase) / playback->speed );
        playback->skipped_time += frame_speed_skip_time_sec;

        const double current_time = get_playback_current_time(playback);
        double waitDuration = nextFrameTimeSinceStartInSeconds - current_time + (double)(readingFrame->repeat_pict) / (2 * frameRate);
        waitDuration -= frame_speed_skip_time_sec;
        double continueTime = clock_sec() + waitDuration;

        add_debug_message(debug_info, debug_video_source, debug_video_type, "  timeOfNextFrame: %f, waitDuration: %f\n \
            Speed Factor: %f, Time Skipped due to Speed on Current Frame: %f\n\n ",
           nextFrameTimeSinceStartInSeconds, waitDuration,
            playback->speed, frame_speed_skip_time_sec);


        pthread_mutex_unlock(alterMutex);
        av_frame_unref(readingFrame);
        if (waitDuration <= 0) {
            continue;
        }
        fsleep_for_sec(waitDuration);
    }

    av_frame_free(&readingFrame);
    return NULL;
}

void jump_to_time(MediaTimeline* timeline, double targetTime) {
    Playback* playback = timeline->playback;
    MediaData* media_data = timeline->mediaData;
    const double originalTime = get_playback_current_time(timeline->playback);
    MediaStream* video_stream = get_media_stream(media_data, AVMEDIA_TYPE_VIDEO);
    if (video_stream == NULL) {
        return;
    }

    AVCodecContext* videoCodecContext = video_stream->info->codecContext;
    SelectionList* videoPackets = video_stream->packets;
    double videoTimeBase = video_stream->timeBase;
    const int64_t targetVideoPTS = targetTime / videoTimeBase;
    AVPacket* packet_get;

    if (targetTime == originalTime) {
        return;
     } else if (targetTime < originalTime) {
        avcodec_flush_buffers(videoCodecContext);
        double testTime = fmax(0.0, targetTime - 30);
        packet_get = (AVPacket*)selection_list_get(videoPackets);
        if (packet_get != NULL) {
            while (packet_get->pts * videoTimeBase > testTime && selection_list_can_move_index(videoPackets, -1)) {
                selection_list_try_move_index(videoPackets, -1);
                packet_get = (AVPacket*)selection_list_get(videoPackets);
                if (packet_get == NULL) {
                    break;
                }
            }
        }
    }

    int64_t finalPTS = -1;
    int readingStatus = 0;
    int nb_decoded = 0;

    /* while (finalPTS < targetVideoPTS && videoPackets->get_index() + 1 < videoPackets->get_length() && (readingStatus > 0 || readingStatus == AVERROR(EAGAIN))  ) { */
    while (1) {
        if (finalPTS >= targetVideoPTS || !selection_list_can_move_index(videoPackets, 1) || (readingStatus < 0 && readingStatus != AVERROR(EAGAIN)) ) {
            break;
        } 

        packet_get = (AVPacket*)selection_list_get(videoPackets);
        if (packet_get == NULL) {
            selection_list_try_move_index(videoPackets, 1);
            continue;
        }

        AVFrame** decodedFrames = decode_video_packet(videoCodecContext, packet_get, &readingStatus, &nb_decoded);
        selection_list_try_move_index(videoPackets, 1);

        while (readingStatus == AVERROR(EAGAIN) && selection_list_can_move_index(videoPackets, 1) && nb_decoded > 0 && decodedFrames != NULL) {
            free_frame_list(decodedFrames, nb_decoded);
            packet_get = (AVPacket*)selection_list_get(videoPackets);
            if (packet_get == NULL) {
                continue;
            }

            decodedFrames = decode_video_packet(videoCodecContext, packet_get, &readingStatus, &nb_decoded);
            selection_list_try_move_index(videoPackets, 1);
        }

        if (readingStatus >= 0) {
            finalPTS = decodedFrames[0]->pts;
        }
        if (decodedFrames != NULL) {
            free_frame_list(decodedFrames, nb_decoded);
        }
    }

    packet_get = (AVPacket*)selection_list_get(videoPackets);
    finalPTS = finalPTS == -1 && packet_get != NULL ? packet_get->pts : finalPTS;
    double timeMoved = (finalPTS * videoTimeBase) - originalTime;
    playback->skipped_time += timeMoved;
}

