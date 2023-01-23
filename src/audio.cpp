
#include "boiler.h"
#include "debug.h"
#include "decode.h"
#include "media.h"
#include <wtime.h>
#include <playheadlist.hpp>
#include <audio.h>
#include <wmath.h>
#include <cstdlib>
#include <threads.h>
#include <macros.h>

extern "C" {
#include <curses.h>
#include <pthread.h>

#include "miniaudio.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

const int AUDIO_BUFFER_SIZE = 8192;
const int MAX_AUDIO_BUFFER_SIZE = 524288;

typedef struct CallbackData {
    MediaPlayer* player;
    pthread_mutex_t* mutex;
    AudioResampler* audioResampler;
} CallbackData;

const char* debug_audio_source = "audio";
const char* debug_audio_type = "debug";

AVFrame** get_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, AVPacket* packet, int* result, int* nb_frames_decoded);
AVFrame** find_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, PlayheadList<AVPacket*>* packet_buffer, int* result, int* nb_frames_decoded);
AVAudioFifo* av_audio_fifo_combine(AVAudioFifo* first, AVAudioFifo* second);

void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    CallbackData* data = (CallbackData*)(pDevice->pUserData);
    pthread_mutex_lock(data->mutex);
    AudioStream* audioStream = data->player->displayCache->audio_stream;

    if (audioStream == nullptr) {
        pthread_mutex_unlock(data->mutex);
        return;
    }

    if (audioStream->can_read(frameCount)) {
        audioStream->read_into(frameCount, (float*)pOutput);
    }

    (void)pInput;
    pthread_mutex_unlock(data->mutex);
}

void* audio_playback_thread(void* args) {
    MediaThreadData* thread_data = (MediaThreadData*)args;
    MediaPlayer* player = thread_data->player;
    pthread_mutex_t* alterMutex = thread_data->alterMutex;
    MediaDebugInfo* debug_info = player->displayCache->debug_info;
    int result;
    MediaStream* audio_media_stream = get_media_stream(player->timeline->mediaData, AVMEDIA_TYPE_AUDIO);
    if (audio_media_stream == NULL) {
        return NULL;
    }


    AVCodecContext* audioCodecContext = audio_media_stream->info->codecContext;
    const int nb_channels = audioCodecContext->ch_layout.nb_channels;

    AudioResampler* audioResampler = get_audio_resampler(&result,     
            &(audioCodecContext->ch_layout), AV_SAMPLE_FMT_FLT, audioCodecContext->sample_rate,
            &(audioCodecContext->ch_layout), audioCodecContext->sample_fmt, audioCodecContext->sample_rate);
    if (audioResampler == NULL) {
        add_debug_message(player->displayCache->debug_info, debug_audio_source, debug_audio_type, "Audio Resampler Allocation Error", "COULD NOT ALLOCATE TEMPORARY AUDIO RESAMPLER");
        return NULL;
    }   

    player->displayCache->audio_stream->init(nb_channels, audioCodecContext->sample_rate);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format  = ma_format_f32;
    config.playback.channels = nb_channels;              
    config.sampleRate = audioCodecContext->sample_rate;           
    config.dataCallback = audioDataCallback;   

    CallbackData userData = { player, alterMutex, audioResampler }; 
   config.pUserData = &userData;   

    ma_result miniAudioLog;
    ma_device audioDevice;
    miniAudioLog = ma_device_init(NULL, &config, &audioDevice);
    if (miniAudioLog != MA_SUCCESS) {
        fprintf(stderr, "%s %d\n", "FAILED TO INITIALIZE AUDIO DEVICE: ", miniAudioLog);
        return NULL;  // Failed to initialize the device.
    }

    Playback* playback = player->timeline->playback;

    sleep_for((long)(audio_media_stream->info->stream->start_time * audio_media_stream->timeBase * SECONDS_TO_NANOSECONDS));
    
    while (player->inUse) {
        pthread_mutex_lock(alterMutex);

        if (playback->is_playing() == false && ma_device_get_state(&audioDevice) == ma_device_state_started) {
            pthread_mutex_unlock(alterMutex);
            miniAudioLog = ma_device_stop(&audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                fprintf(stderr, "%s %d\n", "Failed to stop playback: ", miniAudioLog);
                ma_device_uninit(&audioDevice);
                return NULL;
            };
            pthread_mutex_lock(alterMutex);
        } else if (playback->is_playing() && ma_device_get_state(&audioDevice) == ma_device_state_stopped) {
            pthread_mutex_unlock(alterMutex);
            miniAudioLog = ma_device_start(&audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                fprintf(stderr, "%s %d\n", "Failed to start playback: ", miniAudioLog);
                ma_device_uninit(&audioDevice);
                return NULL;
            };
            pthread_mutex_lock(alterMutex);
        }

        AudioStream* audioStream = player->displayCache->audio_stream;
        if (audio_media_stream->packets->try_step_forward()) {
            
            AVPacket* packet = audio_media_stream->packets->get();
            int result, nb_frames_decoded;
            AVFrame** audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, packet, &result, &nb_frames_decoded);
            if (audioFrames != NULL) {
                if (result >= 0 && nb_frames_decoded > 0) {
                    for (int i = 0; i < nb_frames_decoded; i++) {
                        AVFrame* current_frame = audioFrames[i];
                        float* frameData = (float*)(current_frame->data[0]);
                        audioStream->write(frameData, current_frame->nb_samples);
                    }
                }
                free_frame_list(audioFrames, nb_frames_decoded);
            }
        }


        audioDevice.sampleRate = audioCodecContext->sample_rate * playback->get_speed();
        double current_time = playback->get_time();
        double desync = dabs(audioStream->get_time() - current_time);
        add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Audio Desync", "%s%.2f\n", "Audio Desync Amount: ", desync);

        if (desync > 0.15) {
            if (audioStream->is_time_in_bounds(current_time)) {
                audioStream->set_time(current_time);
            } else {
                audioStream->clear_and_restart_at(current_time);
            }

            move_packet_list_to_pts(audio_media_stream->packets, current_time / audio_media_stream->timeBase);
        }

        pthread_mutex_unlock(alterMutex);
        sleep_for_ms(3);
    }


    free_audio_resampler(audioResampler);
    return NULL;
}

AVFrame** get_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, AVPacket* packet, int* result, int* nb_frames_decoded) {
    AVFrame** rawAudioFrames = decode_audio_packet(audioCodecContext, packet, result, nb_frames_decoded);
    if (rawAudioFrames == NULL || *result < 0 || *nb_frames_decoded == 0) {
        if (rawAudioFrames != NULL) {
            free_frame_list(rawAudioFrames, *nb_frames_decoded);
        }

        return NULL;
    }

    AVFrame** audioFrames = resample_audio_frames(audioResampler, rawAudioFrames, *nb_frames_decoded);
    free_frame_list(rawAudioFrames, *nb_frames_decoded);
    return audioFrames;
}

AVFrame** find_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, PlayheadList<AVPacket*>* audioPackets, int* result, int* nb_frames_decoded) {
    *result = 0;
    AVPacket* currentPacket = audioPackets->get();
    while (currentPacket == nullptr && audioPackets->can_step_forward()) {
        audioPackets->step_forward();
        currentPacket = audioPackets->get();
    }
    if (currentPacket == nullptr) {
        return nullptr;
    }


    AVFrame** audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, currentPacket, result, nb_frames_decoded);
    while (*result == AVERROR(EAGAIN) && audioPackets->can_step_forward()) {
        audioPackets->step_forward();
        if (audioFrames != nullptr) {
            free_frame_list(audioFrames, *nb_frames_decoded);
        }

        currentPacket = audioPackets->get();
        while (currentPacket == nullptr && audioPackets->can_step_forward()) {
            audioPackets->step_forward();
            currentPacket = audioPackets->get();
        }
        if (currentPacket == nullptr) {
            return nullptr;
        }
        audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, currentPacket, result, nb_frames_decoded);
    }

    if (*result < 0 || *nb_frames_decoded == 0) {
        if (audioFrames != nullptr) {
            free_frame_list(audioFrames, *nb_frames_decoded);
        }
        return NULL;
    }

    return audioFrames;
}

float** copy_samples(float** src, int linesize[8], int nb_channels, int nb_samples) {
    float** dst = alloc_samples(linesize, nb_channels, nb_samples);
    av_samples_copy((uint8_t**)dst, (uint8_t *const *)src, 0, 0, nb_samples, nb_channels, AV_SAMPLE_FMT_FLT);
    return dst;
}

float** alloc_samples(int linesize[8], int nb_channels, int nb_samples) {
    float** samples;
    av_samples_alloc_array_and_samples((uint8_t***)(&samples), linesize, nb_channels, nb_samples * 3 / 2, AV_SAMPLE_FMT_FLT, 0);
    return samples; 
}

void free_samples(float** samples) {
    av_free(&samples[0]);
}
float** alterAudioSampleLength(float** originalSamples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples) {
    if (nb_samples < target_nb_samples) {
        return stretchAudioSamples(originalSamples, linesize, nb_samples, nb_channels, target_nb_samples);
    } else if (nb_samples > target_nb_samples) {
        return shrinkAudioSamples(originalSamples, linesize, nb_samples, nb_channels, target_nb_samples);
    } else {
        return copy_samples(originalSamples, linesize, nb_channels, nb_samples);
    }
}

float** stretchAudioSamples(float** originalSamples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples) {
    float** finalValues = alloc_samples(linesize, target_nb_samples, nb_channels);
    double stretchAmount = (double)target_nb_samples / nb_samples;
    float orignalSampleBlocks[nb_samples][nb_channels];
    float finalSampleBlocks[target_nb_samples][nb_channels];
    for (int i = 0; i < nb_samples * nb_channels; i++) {
        orignalSampleBlocks[i / nb_channels][i % nb_channels] = originalSamples[0][i];
    }

    float currentSampleBlock[nb_channels];
    float lastSampleBlock[nb_channels];
    float currentFinalSampleBlockIndex = 0;

    for (int i = 0; i < nb_samples; i++) {
        for (int cpy = 0; cpy < nb_channels; cpy++) {
            currentSampleBlock[cpy] = orignalSampleBlocks[i][cpy];
        }

        if (i != 0) {
            for (int currentChannel = 0; currentChannel < nb_channels; currentChannel++) {
                float sampleStep = (currentSampleBlock[currentChannel] - lastSampleBlock[currentChannel]) / stretchAmount; 
                float currentSampleValue = lastSampleBlock[currentChannel];

                for (int finalSampleBlockIndex = (int)(currentFinalSampleBlockIndex); finalSampleBlockIndex < (int)(currentFinalSampleBlockIndex + stretchAmount); finalSampleBlockIndex++) {
                    finalSampleBlocks[finalSampleBlockIndex][currentChannel] = currentSampleValue;
                    currentSampleValue += sampleStep;
                }

            }
        }

        currentFinalSampleBlockIndex += stretchAmount;
        for (int cpy = 0; cpy < nb_channels; cpy++) {
            lastSampleBlock[cpy] = currentSampleBlock[cpy];
        }
    }

    for (int i = 0; i < target_nb_samples * nb_channels; i++) {
        finalValues[0][i] = finalSampleBlocks[i / nb_channels][i % nb_channels];
    }
    return finalValues;
}

float** shrinkAudioSamples(float** originalSamples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples) {
    float** finalValues = alloc_samples(linesize, target_nb_samples, nb_channels);
    double stretchAmount = (double)target_nb_samples / nb_samples;
    float stretchStep = 1 / stretchAmount;
    float orignalSampleBlocks[nb_samples][nb_channels];
    float finalSampleBlocks[target_nb_samples][nb_channels];
    for (int i = 0; i < nb_samples * nb_channels; i++) {
        orignalSampleBlocks[i / nb_channels][i % nb_channels] = originalSamples[0][i];
    }

    float currentOriginalSampleBlockIndex = 0;

    for (int i = 0; i < target_nb_samples; i++) {
        float average = 0.0;
        for (int ichannel = 0; ichannel < nb_channels; ichannel++) {
           average = 0.0;
           for (int origi = (int)(currentOriginalSampleBlockIndex); origi < (int)(currentOriginalSampleBlockIndex + stretchStep); origi++) {
               average += orignalSampleBlocks[origi][ichannel];
           }
           average /= stretchStep;
           finalSampleBlocks[i][ichannel] = average;
        }

        currentOriginalSampleBlockIndex += stretchStep;
    }

    for (int i = 0; i < target_nb_samples * nb_channels; i++) {
        finalValues[0][i] = finalSampleBlocks[i / nb_channels][i % nb_channels];
    }
    return finalValues;
}

uint8_t float_sample_to_uint8(float num) {
    return (255.0 / 2.0) * (num + 1.0);
}

float uint8_sample_to_float(uint8_t sample) {
    return ((float)sample - 128.0) / 128.0;
}
