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

#define MAX_AUDIO_ASYNC_TIME_SECONDS 0.15
#include <cstdint>

uint8_t float_sample_to_uint8(float num);
float uint8_sample_to_float(uint8_t sample);

float** alloc_samples(int linesize[8], int nb_channels, int nb_samples);
float** copy_samples(float** src, int linesize[8], int nb_channels, int nb_samples);
void free_samples(float** samples);

float** alterAudioSampleLength(float** originalSamples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
float** stretchAudioSamples(float** originalSamples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
float** shrinkAudioSamples(float** originalSamples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
#endif
