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

#include <vector>

std::vector<float> audio_to_mono(std::vector<float>& samples, int nb_channels);
void audio_normalize(std::vector<float>& samples);
void audio_bound_volume(std::vector<float>& samples, float max);

#endif
