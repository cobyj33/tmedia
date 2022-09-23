#include "boiler.h"
#include "decode.h"
#include "doublelinkedlist.hpp"
#include "media.h"
#include <audio.h>
#include <cstdlib>
#include <libavutil/error.h>
#include <libavutil/samplefmt.h>
#include <thread>
#include <chrono>
#include <macros.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define AUDIO_FIFO_BUFFER_SIZE 8192
#define MAX_AUDIO_FIFO_BUFFER_SIZE 524288

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

typedef struct CallbackData {
    MediaPlayer* player;
    std::mutex* mutex;
    AudioResampler* audioResampler;
    std::chrono::steady_clock::time_point last_time;
} CallbackData;

AVFrame** get_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, AVPacket* packet, int* result, int* nb_frames_decoded);
AVFrame** find_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, AVPacket** packet_buffer, int packet_buffer_length, int* result, int* nb_frames_decoded);
AVFrame** find_final_audio_frames_dll(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, DoubleLinkedList<AVPacket*>* packet_buffer, int* result, int* nb_frames_decoded);

const char* debug_audio_source = "audio";
const char* debug_audio_type = "debug";

void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        CallbackData* callbackData = static_cast<CallbackData*>(pDevice->pUserData);
        std::mutex* alterMutex = callbackData->mutex;
        std::lock_guard<std::mutex> alterLock{*alterMutex};


        MediaPlayer* player = callbackData->player;
        AudioResampler* audioResampler = callbackData->audioResampler;
        Playback* playback = player->timeline->playback;
        MediaDebugInfo* debug_info = player->displayCache->debug_info;
        MediaStream* audio_stream = get_media_stream(player->timeline->mediaData, AVMEDIA_TYPE_AUDIO);
        if (audio_stream == nullptr || !player->inUse) {
            return;
        }

        AVCodecContext* audioCodecContext = audio_stream->info->codecContext; 
        DoubleLinkedList<AVPacket*>* audioPackets = audio_stream->packets;
        pDevice->masterVolumeFactor = playback->volume;

        int samplesToSkip = 0;
        const double stretchAmount = 1 / playback->speed;
        /* const int bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT); */
        const double since_last_call = (double)std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::steady_clock::now() - callbackData->last_time)).count() / SECONDS_TO_NANOSECONDS;
        audio_stream->streamTime += since_last_call;
        const double audio_async_time = std::abs(audio_stream->streamTime  - playback->time); 
        const int64_t targetAudioPTS = (int64_t)(audio_stream->streamTime / audio_stream->timeBase);

        add_debug_message(debug_info, debug_audio_source, debug_audio_type, "AUDIO DEBUG: \n\n Playback: \n  Master Time: %f \n Audio Time: %f \n Unsync Amount: %f \n    Speed: %f \n    Playing: %s\n Volume: %f\n", playback->time, audio_stream->streamTime, std::abs(audio_stream->streamTime - playback->time), playback->speed, playback->playing ? "true" : "false", playback->volume);

        if (audio_async_time >= MAX_AUDIO_ASYNC_TIME_SECONDS) {
            audio_stream->streamTime = playback->time;
        }

        move_packet_list_to_pts(audioPackets, targetAudioPTS);

        int result, nb_frames_decoded;
        AVFrame** audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, audioPackets->get(), &result, &nb_frames_decoded);
        while (result == AVERROR(EAGAIN) && audioPackets->can_move_index(1)) {
            audioPackets->try_move_index(1);
            if (audioFrames != nullptr) {
                free_frame_list(audioFrames, nb_frames_decoded);
            }

            audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, audioPackets->get(), &result, &nb_frames_decoded);
        }

        if (audioFrames == nullptr) {
            add_debug_message(debug_info, debug_audio_source, debug_audio_type, "COULD NOT RESAMPLE AUDIO FRAMES");
            return;
        }

        AVAudioFifo* tempBuffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, audioCodecContext->ch_layout.nb_channels, AUDIO_FIFO_BUFFER_SIZE);
        if (tempBuffer == NULL) {
            add_debug_message(debug_info, debug_audio_source, debug_audio_type, "COULD NOT ALLOCATE TEMPORARY AUDIO FIFO");
            return;
        }

        int audioFrameIndex = 0;
        samplesToSkip = std::round((audio_stream->streamTime - audioFrames[audioFrameIndex]->pts * audio_stream->timeBase) * audioCodecContext->sample_rate / stretchAmount); 
        AVFrame* currentAudioFrame = audioFrames[audioFrameIndex];
        int samplesWritten = 0;
        int framesRead = 0;

        bool shouldResampleAudioFrames = playback->speed != 1.0;
        AudioResampler* inTimeAudioResampler = nullptr;
        if (shouldResampleAudioFrames) {
            inTimeAudioResampler = get_audio_resampler(&result,
            &(audioCodecContext->ch_layout), AV_SAMPLE_FMT_FLT,  audioCodecContext->sample_rate / stretchAmount,
            &(audioCodecContext->ch_layout) , AV_SAMPLE_FMT_FLT, audioCodecContext->sample_rate);
            if (inTimeAudioResampler == nullptr) {
                add_debug_message(debug_info, debug_audio_source, debug_audio_type, "COULD NOT ALLOCATE AND INITIALIZE IN TIME AUDIO RESAMPLER FOR AUDIO PLAYBACK");
                shouldResampleAudioFrames = false;
            }
        }


        while (samplesWritten < frameCount + samplesToSkip && av_audio_fifo_space(tempBuffer) > currentAudioFrame->nb_samples) {
            framesRead++;
            add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Frame %d:\n     ", framesRead);

            if (shouldResampleAudioFrames) {
                int samplesToWrite = (int)(currentAudioFrame->nb_samples * stretchAmount);
                float** originalValues = copy_samples((float**)currentAudioFrame->data, currentAudioFrame->linesize, currentAudioFrame->ch_layout.nb_channels, currentAudioFrame->nb_samples);
                float** resampledValues = alloc_samples(currentAudioFrame->linesize, currentAudioFrame->ch_layout.nb_channels, currentAudioFrame->nb_samples);
                float** finalValues = alterAudioSampleLength(originalValues, currentAudioFrame->linesize, currentAudioFrame->nb_samples, currentAudioFrame->ch_layout.nb_channels, samplesToWrite);
                int conversionResult = swr_convert(inTimeAudioResampler->context, (uint8_t**)(resampledValues), currentAudioFrame->nb_samples, (const uint8_t**)(finalValues), samplesToWrite);

                if (conversionResult < 0) {
                    add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Could not correctly perform SWR resampling\n");
                    samplesWritten += av_audio_fifo_write(tempBuffer, (void**)finalValues, samplesToWrite);
                } else {
                    samplesWritten += av_audio_fifo_write(tempBuffer, (void**)resampledValues, currentAudioFrame->nb_samples);
                }

                 av_freep(&originalValues[0]);
                 av_freep(&finalValues[0]);
                 av_freep(&resampledValues[0]);
            } else  {
                add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Feeding unaltered audio data\n");
                samplesWritten += av_audio_fifo_write(tempBuffer, (void**)currentAudioFrame->data, currentAudioFrame->nb_samples);
            }

            av_frame_free(&currentAudioFrame);
            if (audioFrameIndex + 1 < nb_frames_decoded) {
                audioFrameIndex++;
                currentAudioFrame = audioFrames[audioFrameIndex];
            } else {
                audioFrameIndex = 0;
                if (audioPackets->can_move_index(1)) {
                    audioPackets->try_move_index(1);
                    audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, audioPackets->get(), &result, &nb_frames_decoded);
                    while (result == AVERROR(EAGAIN) && audioPackets->can_move_index(1)) {
                        audioPackets->try_move_index(1);
                        if (audioFrames != nullptr) {
                            free_frame_list(audioFrames, nb_frames_decoded);
                        }

                        audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, audioPackets->get(), &result, &nb_frames_decoded);
                    }

                    if (audioFrames == nullptr) {
                        add_debug_message(debug_info, debug_audio_source, debug_audio_type, "COULD NOT RESAMPLE AUDIO FRAMES");
                        break;
                    }

                    currentAudioFrame = audioFrames[audioFrameIndex];
                    continue;
                } 
                break;
            }
        }

        const int audioSamplesToOutput = std::min((int)frameCount, samplesWritten);
        av_audio_fifo_drain(tempBuffer, samplesToSkip);
        av_audio_fifo_read(tempBuffer, &pOutput, audioSamplesToOutput);

        add_debug_message(debug_info, debug_audio_source, debug_audio_type, "Samples Written: %d, Samples Requested: %d, Samples Skipped: %d, Frames Read: %d Device Sample Rate: %d\n ", samplesWritten, (int)frameCount, samplesToSkip, framesRead, pDevice->sampleRate);

        if (inTimeAudioResampler != nullptr) {
           free_audio_resampler(inTimeAudioResampler); 
        }
        if (audioFrames != nullptr) {
           free_frame_list(audioFrames, nb_frames_decoded);
        }
        av_audio_fifo_free(tempBuffer);
        
        callbackData->last_time = std::chrono::steady_clock::now();
        (void)pInput;
    }

int audio_playback_thread(MediaPlayer* player, std::mutex* alterMutex) {
    int result;

    MediaStream* audio_stream = get_media_stream(player->timeline->mediaData, AVMEDIA_TYPE_AUDIO);
    if (audio_stream == nullptr) {
        return EXIT_FAILURE;
    }


    AVCodecContext* audioCodecContext = audio_stream->info->codecContext;
    AudioResampler* audioResampler = get_audio_resampler(&result,     
            &(audioCodecContext->ch_layout), AV_SAMPLE_FMT_FLT, audioCodecContext->sample_rate,
            &(audioCodecContext->ch_layout), audioCodecContext->sample_fmt, audioCodecContext->sample_rate);
    AVAudioFifo* audioFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, audioCodecContext->ch_layout.nb_channels, 8192);
    if (audioResampler == NULL || audioFifo == NULL) {
        return EXIT_FAILURE;
    }   



    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format  = ma_format_f32;
    config.playback.channels = audioCodecContext->ch_layout.nb_channels;              
    config.sampleRate = audioCodecContext->sample_rate;           
    config.dataCallback = audioDataCallback;   

    CallbackData userData = { player, alterMutex, audioResampler, std::chrono::steady_clock::now()}; 
   config.pUserData = &userData;   

    ma_result miniAudioLog;
    ma_device audioDevice;
    miniAudioLog = ma_device_init(NULL, &config, &audioDevice);
    if (miniAudioLog != MA_SUCCESS) {
        std::cout << "FAILED TO INITIALIZE AUDIO DEVICE: " << miniAudioLog << std::endl;
        return EXIT_FAILURE;  // Failed to initialize the device.
    }

    Playback* playback = player->timeline->playback;

    std::this_thread::sleep_for(std::chrono::nanoseconds((int)(audio_stream->info->stream->start_time * audio_stream->timeBase * SECONDS_TO_NANOSECONDS)));
    
    while (player->inUse) {
        if (playback->playing == false && ma_device_get_state(&audioDevice) == ma_device_state_started) {
            miniAudioLog = ma_device_stop(&audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                std::cout << "Failed to stop playback: " << miniAudioLog << std::endl;
                ma_device_uninit(&audioDevice);
                return EXIT_FAILURE;
            };
        } else if (playback->playing && ma_device_get_state(&audioDevice) == ma_device_state_stopped) {
            miniAudioLog = ma_device_start(&audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                std::cout << "Failed to start playback: " << miniAudioLog << std::endl;
                ma_device_uninit(&audioDevice);
                return EXIT_FAILURE;
            };
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    free_audio_resampler(audioResampler);
    return EXIT_SUCCESS;
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


AVFrame** find_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, AVPacket** packet_buffer, int packet_buffer_length, int* result, int* nb_frames_decoded) {
    AVFrame** audioFrames = nullptr;
    for (int i = 0; i < packet_buffer_length; i++) {
        audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, packet_buffer[i], result, nb_frames_decoded);
        if (*result >= 0 && *nb_frames_decoded > 0) {
            return audioFrames;
        }

        free_frame_list(audioFrames, *nb_frames_decoded);
    }

    return audioFrames;
}

AVFrame** get_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler* audioResampler, AVPacket* packet, int* result, int* nb_frames_decoded) {
    AVFrame** rawAudioFrames = decode_audio_packet(audioCodecContext, packet, result, nb_frames_decoded);
    if (rawAudioFrames == nullptr || *result < 0 || *nb_frames_decoded == 0) {
        return nullptr;
    }

    AVFrame** audioFrames = resample_audio_frames(audioResampler, rawAudioFrames, *nb_frames_decoded);
    free_frame_list(rawAudioFrames, *nb_frames_decoded);

    return audioFrames;
}
