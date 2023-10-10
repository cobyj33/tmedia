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
#include <vector>

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

std::vector<float> audio_uint8_buffer_to_float(std::vector<uint8_t>& samples);
std::vector<float> audio_to_mono(std::vector<float>& samples, int nb_channels);
void audio_normalize(std::vector<float>& samples);

#endif
