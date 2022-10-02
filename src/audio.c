#include "boiler.h"
#include "debug.h"
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
AVAudioFifo* av_audio_fifo_combine(AVAudioFifo* first, AVAudioFifo* second);


void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const char* debug_audio_source = "audio";
    const char* debug_audio_type = "debug";

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
    const int nb_channels = audioCodecContext->ch_layout.nb_channels;

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
        add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Audio Resampling Error", "%s", "COULD NOT RESAMPLE AUDIO FRAMES");
        return;
    }

    AVAudioFifo* tempBuffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, nb_channels, AUDIO_FIFO_BUFFER_SIZE);
    if (tempBuffer == NULL) {
        add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Audio Fifo Allocation Error", "COULD NOT ALLOCATE TEMPORARY AUDIO FIFO");
        return;
    }

    int target_buffer_size = (frameCount + samplesToSkip);
    float* samples = (float*)malloc(sizeof(float) * target_buffer_size * nb_channels);
    int saved_samples_written = 0;
    if (samples == NULL) {
        fprintf(stderr, "%s", "Could not allocate saved samples");
        av_audio_fifo_free(tempBuffer);
        return;
    }
    for (int i = 0; i < target_buffer_size * nb_channels; i++) {
        samples[i] = 0.0f;
    }

    int audioFrameIndex = 0;
    samplesToSkip = round((stream_time - audioFrames[audioFrameIndex]->pts * audio_stream->timeBase) * audioCodecContext->sample_rate / stretchAmount); 
    AVFrame* currentAudioFrame = audioFrames[audioFrameIndex];
    int samplesWritten = 0;
    int framesRead = 0;

    while (samplesWritten < target_buffer_size && av_audio_fifo_space(tempBuffer) > currentAudioFrame->nb_samples) {
        framesRead++;

        const int cutoff_size = 125;
        if ( samplesWritten + currentAudioFrame->nb_samples > samplesToSkip && samplesWritten < samplesToSkip ) { 
            const int offset = samplesToSkip - samplesWritten;
            for (int i = 0; i < cutoff_size && (offset + i) < currentAudioFrame->nb_samples; i++) {
                for (int s = 0; s < currentAudioFrame->ch_layout.nb_channels; s++) {
                    ((float*)(currentAudioFrame->data[0]))[ (offset + i) * nb_channels + s] *= i / (double)cutoff_size;
                }
            }
        } else if (samplesWritten + currentAudioFrame->nb_samples > target_buffer_size) {

            int fade_out_length = i32min(cutoff_size, target_buffer_size - samplesWritten);
            for (int i = 0; i < fade_out_length && i < currentAudioFrame->nb_samples; i++) {
                for (int s = 0; s < currentAudioFrame->ch_layout.nb_channels; s++) {
                    ((float*)(currentAudioFrame->data[0]))[i * nb_channels + s] *= 1.0 - (i / (double)(fade_out_length));
                }
            }
        }

        samplesWritten += av_audio_fifo_write(tempBuffer, (void**)currentAudioFrame->data, currentAudioFrame->nb_samples);

        if (samplesWritten >= samplesToSkip) {
            int current_audio_frame_index = samplesWritten - samplesToSkip < currentAudioFrame->nb_samples ? samplesWritten - samplesToSkip : 0;
            while (current_audio_frame_index < currentAudioFrame->nb_samples && saved_samples_written < target_buffer_size) {
                for (int s = 0; s < currentAudioFrame->ch_layout.nb_channels; s++) {
                    samples[saved_samples_written * nb_channels + s] = ((float*)(currentAudioFrame->data[0]))[current_audio_frame_index * nb_channels + s];
                }
                saved_samples_written++;
                current_audio_frame_index++;
            }
        }

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
                    add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Audio Resampling Error", "COULD NOT RESAMPLE AUDIO FRAMES");
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
    av_audio_fifo_peek(tempBuffer, &pOutput, audioSamplesToOutput);
    MediaDisplayCache* cache = player->displayCache;

    if (cache->last_samples != NULL) {
        free(cache->last_samples);
        cache->nb_last_samples = 0;
        cache->nb_channels = 0;
    }

    cache->last_samples = samples;
    cache->nb_last_samples = saved_samples_written;
    cache->nb_channels = nb_channels;

    add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Sampling Information", "Samples Written: %d, Samples Requested: %d, Samples Skipped: %d, Frames Read: %d Device Sample Rate: %d\n ", samplesWritten, (int)frameCount, samplesToSkip, framesRead, pDevice->sampleRate);

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
    const int nb_channels = audioCodecContext->ch_layout.nb_channels;

    AudioResampler* audioResampler = get_audio_resampler(&result,     
            &(audioCodecContext->ch_layout), AV_SAMPLE_FMT_FLT, audioCodecContext->sample_rate,
            &(audioCodecContext->ch_layout), audioCodecContext->sample_fmt, audioCodecContext->sample_rate);
    AVAudioFifo* audioFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, nb_channels, 8192);
    if (audioResampler == NULL || audioFifo == NULL) {
        return NULL;
    }   
    player->displayCache->nb_channels = nb_channels;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format  = ma_format_f32;
    config.playback.channels = nb_channels;              
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

