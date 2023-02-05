
#include <cstdlib>

#include "audio.h"
#include "wmath.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#include <libavutil/mem.h>
}

uint8_t normalized_float_sample_to_uint8(float num) {
    return (255.0 / 2.0) * (num + 1.0);
}

float uint8_sample_to_normalized_float(uint8_t sample) {
    return ((float)sample - 128.0) / 128.0;
}

float** alloc_samples(int linesize[8], int nb_channels, int nb_samples) {
    float** samples;
    av_samples_alloc_array_and_samples((uint8_t***)(&samples), linesize, nb_channels, nb_samples * 3 / 2, AV_SAMPLE_FMT_FLT, 0);
    return samples; 
}

float** copy_samples(float** src, int linesize[8], int nb_channels, int nb_samples) {
    float** dst = alloc_samples(linesize, nb_channels, nb_samples);
    av_samples_copy((uint8_t**)dst, (uint8_t *const *)src, 0, 0, nb_samples, nb_channels, AV_SAMPLE_FMT_FLT);
    return dst;
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

