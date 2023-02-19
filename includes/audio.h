#ifndef ASCII_VIDEO_AUDIO_IMPLEMENTATION
#define ASCII_VIDEO_AUDIO_IMPLEMENTATION
/**
 * @file audio.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Common functions for audio manipulation in ascii_video
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 */

#include <cstdint>

uint8_t normalized_float_sample_to_uint8(float num);
float uint8_sample_to_normalized_float(uint8_t sample);

float** alloc_samples(int linesize[8], int nb_channels, int nb_samples);
float** copy_samples(float** src, int linesize[8], int nb_channels, int nb_samples);
void free_samples(float** samples);

float** alter_audio_sample_length(float** original_samples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
float** stretch_audio_samples(float** original_samples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
float** shrink_audio_samples(float** original_samples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
#endif
