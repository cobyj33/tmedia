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

/**
 * @brief Takes a normalized float sample (-1 < num < 1) and converts it to an 8 bit unsigned integer. This is done where -1 maps to 0 and 1 maps to 255.
 * 
 * @param num A normalized float value between -1 and 1 inclusive
 * @return A unsigned 8 bit integer value representing the range between -1 and 1
 */
uint8_t normalized_float_sample_to_uint8(float num);

/**
 * @brief Takes an 8 bit unsigned integer and converts it to a normalized float between -1 and 1 inclusive. This is done where 0 maps to -1 and 255 maps to 1.
 * 
 * @param num A unsigned 8 bit integer value
 * @return A normalized float value between -1 and 1 inclusive
 */
float uint8_sample_to_normalized_float(uint8_t sample);

float** alloc_samples(int linesize[8], int nb_channels, int nb_samples);
float** copy_samples(float** src, int linesize[8], int nb_channels, int nb_samples);
void free_samples(float** samples);

float** alter_audio_sample_length(float** original_samples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
float** stretch_audio_samples(float** original_samples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
float** shrink_audio_samples(float** original_samples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
#endif
