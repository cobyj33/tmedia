
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

float** alter_audio_sample_length(float** original_samples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples) {
    if (nb_samples < target_nb_samples) {
        return stretch_audio_samples(original_samples, linesize, nb_samples, nb_channels, target_nb_samples);
    } else if (nb_samples > target_nb_samples) {
        return shrink_audio_samples(original_samples, linesize, nb_samples, nb_channels, target_nb_samples);
    } else {
        return copy_samples(original_samples, linesize, nb_channels, nb_samples);
    }
}

float** stretch_audio_samples(float** original_samples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples) {
    float** final_values = alloc_samples(linesize, target_nb_samples, nb_channels);
    double stretch_amount = (double)target_nb_samples / nb_samples;
    float original_sample_blocks[nb_samples][nb_channels];
    float final_sample_blocks[target_nb_samples][nb_channels];
    for (int i = 0; i < nb_samples * nb_channels; i++) {
        original_sample_blocks[i / nb_channels][i % nb_channels] = original_samples[0][i];
    }

    float current_sample_block[nb_channels];
    float last_sample_block[nb_channels];
    float current_final_sample_block_index = 0;

    for (int i = 0; i < nb_samples; i++) {
        for (int cpy = 0; cpy < nb_channels; cpy++) {
            current_sample_block[cpy] = original_sample_blocks[i][cpy];
        }

        if (i != 0) {
            for (int current_channel = 0; current_channel < nb_channels; current_channel++) {
                float sampleStep = (current_sample_block[current_channel] - last_sample_block[current_channel]) / stretch_amount; 
                float current_sample_value = last_sample_block[current_channel];

                for (int final_sample_block_index = (int)(current_final_sample_block_index); final_sample_block_index < (int)(current_final_sample_block_index + stretch_amount); final_sample_block_index++) {
                    final_sample_blocks[final_sample_block_index][current_channel] = current_sample_value;
                    current_sample_value += sampleStep;
                }

            }
        }

        current_final_sample_block_index += stretch_amount;
        for (int cpy = 0; cpy < nb_channels; cpy++) {
            last_sample_block[cpy] = current_sample_block[cpy];
        }
    }

    for (int i = 0; i < target_nb_samples * nb_channels; i++) {
        final_values[0][i] = final_sample_blocks[i / nb_channels][i % nb_channels];
    }
    return final_values;
}

float** shrink_audio_samples(float** original_samples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples) {
    float** final_values = alloc_samples(linesize, target_nb_samples, nb_channels);
    double stretch_amount = (double)target_nb_samples / nb_samples;
    float stretch_step = 1 / stretch_amount;
    float original_sample_blocks[nb_samples][nb_channels];
    float final_sample_blocks[target_nb_samples][nb_channels];
    for (int i = 0; i < nb_samples * nb_channels; i++) {
        original_sample_blocks[i / nb_channels][i % nb_channels] = original_samples[0][i];
    }

    float current_original_sample_block_index = 0;

    for (int i = 0; i < target_nb_samples; i++) {
        float average = 0.0;
        for (int ichannel = 0; ichannel < nb_channels; ichannel++) {
           average = 0.0;
           for (int origi = (int)(current_original_sample_block_index); origi < (int)(current_original_sample_block_index + stretch_step); origi++) {
               average += original_sample_blocks[origi][ichannel];
           }
           average /= stretch_step;
           final_sample_blocks[i][ichannel] = average;
        }

        current_original_sample_block_index += stretch_step;
    }

    for (int i = 0; i < target_nb_samples * nb_channels; i++) {
        final_values[0][i] = final_sample_blocks[i / nb_channels][i % nb_channels];
    }
    return final_values;
}

