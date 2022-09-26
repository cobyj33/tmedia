#include "boiler.h"
#include "decode.h"
#include "media.h"
#include <wtime.h>
#include <selectionlist.h>
#include <audio.h>
#include <wmath.h>
#include <stdlib.h>
#include <pthread.h>
#include <threads.h>
#include <macros.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>

const int AUDIO_FIFO_BUFFER_SIZE = 8192;
const int MAX_AUDIO_FIFO_BUFFER_SIZE = 524288;

typedef struct CallbackData {
    MediaPlayer* player;
    pthread_mutex_t* mutex;
    AudioResampler* audioResampler;
} CallbackData;


AVFrame** get_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, AVPacket* packet, int* result, int* nb_frames_decoded);
AVFrame** find_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, SelectionList* packet_buffer, int* result, int* nb_frames_decoded);

const char* debug_audio_source = "audio";
const char* debug_audio_type = "debug";

void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    CallbackData* callbackData = (CallbackData*)(pDevice->pUserData);
    pthread_mutex_t* alterMutex = callbackData->mutex;
    pthread_mutex_lock(alterMutex);

    MediaPlayer* player = callbackData->player;
    AudioResampler* audioResampler = callbackData->audioResampler;
    Playback* playback = player->timeline->playback;
    MediaDebugInfo* debug_info = player->displayCache->debug_info;
    MediaStream* audio_stream = get_media_stream(player->timeline->mediaData, AVMEDIA_TYPE_AUDIO);
    if (audio_stream == NULL || !player->inUse) {
        return;
    }

    AVCodecContext* audioCodecContext = audio_stream->info->codecContext; 
    SelectionList* audioPackets = audio_stream->packets;
    pDevice->masterVolumeFactor = playback->volume;

    int samplesToSkip = 0;
    const double stretchAmount = 1 / playback->speed;
    const double stream_time = get_playback_current_time(playback);

    const int64_t targetAudioPTS = (int64_t)(stream_time / audio_stream->timeBase);

    move_packet_list_to_pts(audioPackets, targetAudioPTS);

    int result, nb_frames_decoded;
    AVFrame** audioFrames = find_final_audio_frames(audioCodecContext, audioResampler, audioPackets, &result, &nb_frames_decoded);
    if (audioFrames == NULL) {
        add_debug_message(debug_info, debug_audio_source, debug_audio_type, "COULD NOT RESAMPLE AUDIO FRAMES");
        return;
    }

    AVAudioFifo* tempBuffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, audioCodecContext->ch_layout.nb_channels, AUDIO_FIFO_BUFFER_SIZE);
    if (tempBuffer == NULL) {
        add_debug_message(debug_info, debug_audio_source, debug_audio_type, "COULD NOT ALLOCATE TEMPORARY AUDIO FIFO");
        return;
    }

    int audioFrameIndex = 0;
    samplesToSkip = round((stream_time - audioFrames[audioFrameIndex]->pts * audio_stream->timeBase) * audioCodecContext->sample_rate / stretchAmount); 
    AVFrame* currentAudioFrame = audioFrames[audioFrameIndex];
    int samplesWritten = 0;
    int framesRead = 0;

    while (samplesWritten < frameCount + samplesToSkip && av_audio_fifo_space(tempBuffer) > currentAudioFrame->nb_samples) {
        framesRead++;
        samplesWritten += av_audio_fifo_write(tempBuffer, (void**)currentAudioFrame->data, currentAudioFrame->nb_samples);

        av_frame_free(&currentAudioFrame);
        if (audioFrameIndex + 1 < nb_frames_decoded) {
            audioFrameIndex++;
            currentAudioFrame = audioFrames[audioFrameIndex];
        } else {
            audioFrameIndex = 0;
            if (selection_list_can_move_index(audioPackets, 1)) {
                selection_list_try_move_index(audioPackets, 1);
                audioFrames = find_final_audio_frames(audioCodecContext, audioResampler, audioPackets, &result, &nb_frames_decoded);
                if (audioFrames == NULL) {
                    add_debug_message(debug_info, debug_audio_source, debug_audio_type, "COULD NOT RESAMPLE AUDIO FRAMES");
                    break;
                }

                currentAudioFrame = audioFrames[audioFrameIndex];
                continue;
            } 
            break;
        }
    }

    const int audioSamplesToOutput = i32min((int)frameCount, samplesWritten);
    av_audio_fifo_drain(tempBuffer, samplesToSkip);
    av_audio_fifo_read(tempBuffer, &pOutput, audioSamplesToOutput);

    add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Samples Written: %d, Samples Requested: %d, Samples Skipped: %d, Frames Read: %d Device Sample Rate: %d\n ", samplesWritten, (int)frameCount, samplesToSkip, framesRead, pDevice->sampleRate);

    if (audioFrames != NULL) {
       free_frame_list(audioFrames, nb_frames_decoded);
    }
    av_audio_fifo_free(tempBuffer);
    
    (void)pInput;
    pthread_mutex_unlock(alterMutex);
}

void* audio_playback_thread(void* args) {
    MediaThreadData* thread_data = (MediaThreadData*)args;
    MediaPlayer* player = thread_data->player;
    pthread_mutex_t* alterMutex = thread_data->alterMutex;
    int result;

    MediaStream* audio_stream = get_media_stream(player->timeline->mediaData, AVMEDIA_TYPE_AUDIO);
    if (audio_stream == NULL) {
        return NULL;
    }


    AVCodecContext* audioCodecContext = audio_stream->info->codecContext;
    AudioResampler* audioResampler = get_audio_resampler(&result,     
            &(audioCodecContext->ch_layout), AV_SAMPLE_FMT_FLT, audioCodecContext->sample_rate,
            &(audioCodecContext->ch_layout), audioCodecContext->sample_fmt, audioCodecContext->sample_rate);
    AVAudioFifo* audioFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, audioCodecContext->ch_layout.nb_channels, 8192);
    if (audioResampler == NULL || audioFifo == NULL) {
        return NULL;
    }   



    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format  = ma_format_f32;
    config.playback.channels = audioCodecContext->ch_layout.nb_channels;              
    config.sampleRate = audioCodecContext->sample_rate;           
    config.dataCallback = audioDataCallback;   

    CallbackData userData = { player, alterMutex, audioResampler}; 
   config.pUserData = &userData;   

    ma_result miniAudioLog;
    ma_device audioDevice;
    miniAudioLog = ma_device_init(NULL, &config, &audioDevice);
    if (miniAudioLog != MA_SUCCESS) {
        fprintf(stderr, "%s %d\n", "FAILED TO INITIALIZE AUDIO DEVICE: ", miniAudioLog);
        return NULL;  // Failed to initialize the device.
    }

    Playback* playback = player->timeline->playback;

    sleep_for((long)(audio_stream->info->stream->start_time * audio_stream->timeBase * SECONDS_TO_NANOSECONDS));
    
    while (player->inUse) {
        if (playback->playing == 0 && ma_device_get_state(&audioDevice) == ma_device_state_started) {
            miniAudioLog = ma_device_stop(&audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                fprintf(stderr, "%s %d\n", "Failed to stop playback: ", miniAudioLog);
                ma_device_uninit(&audioDevice);
                return NULL;
            };
        } else if (playback->playing && ma_device_get_state(&audioDevice) == ma_device_state_stopped) {
            miniAudioLog = ma_device_start(&audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                fprintf(stderr, "%s %d\n", "Failed to start playback: ", miniAudioLog);
                ma_device_uninit(&audioDevice);
                return NULL;
            };
        }

        sleep_for_ms(1);
    }

    free_audio_resampler(audioResampler);
    return NULL;
}

AVFrame** get_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, AVPacket* packet, int* result, int* nb_frames_decoded) {
    AVFrame** rawAudioFrames = decode_audio_packet(audioCodecContext, packet, result, nb_frames_decoded);
    if (rawAudioFrames == NULL || *result < 0 || *nb_frames_decoded == 0) {
        return NULL;
    }

    AVFrame** audioFrames = resample_audio_frames(audioResampler, rawAudioFrames, *nb_frames_decoded);
    free_frame_list(rawAudioFrames, *nb_frames_decoded);

    return audioFrames;
}

AVFrame** find_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, SelectionList* audioPackets, int* result, int* nb_frames_decoded) {
    *result = 0;
    AVPacket* currentPacket = (AVPacket*)selection_list_get(audioPackets);
    while (currentPacket == NULL && selection_list_can_move_index(audioPackets, 1)) {
        selection_list_try_move_index(audioPackets, 1);
        currentPacket = (AVPacket*)selection_list_get(audioPackets);
    }
    if (currentPacket == NULL) {
        return NULL;
    }


    AVFrame** audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, currentPacket, result, nb_frames_decoded);
    while (*result == AVERROR(EAGAIN) && selection_list_can_move_index(audioPackets, 1)) {
        selection_list_try_move_index(audioPackets, 1);
        if (audioFrames != NULL) {
            free_frame_list(audioFrames, *nb_frames_decoded);
        }

        currentPacket = (AVPacket*)selection_list_get(audioPackets);
        while (currentPacket == NULL && selection_list_can_move_index(audioPackets, 1)) {
            selection_list_try_move_index(audioPackets, 1);
            currentPacket = (AVPacket*)selection_list_get(audioPackets);
        }
        if (currentPacket == NULL) {
            return NULL;
        }
        audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, currentPacket, result, nb_frames_decoded);
    }

    if (*result < 0 || *nb_frames_decoded == 0) {
        if (audioFrames != NULL) {
            free_frame_list(audioFrames, *nb_frames_decoded);
        }
        return NULL;
    }

    return audioFrames;
}
