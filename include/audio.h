#ifndef TMEDIA_AUDIO_H
#define TMEDIA_AUDIO_H
/**
 * @file audio.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Common functions for audio manipulation in tmedia
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
*/

#include <vector>

std::vector<float> audio_to_mono(std::vector<float>& frames, int nb_channels);
void audio_to_monoip(std::vector<float>& frames, int nb_channels);
void audio_normalize(std::vector<float>& frames, int nb_channels);
void audio_bound_volume(std::vector<float>& frames, int nb_channels, float max);

#endif
