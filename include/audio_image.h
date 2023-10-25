#ifndef ASCII_VIDEO_AUDIO_IMAGE_H
#define ASCII_VIDEO_AUDIO_IMAGE_H

#include "pixeldata.h"
#include <memory>

PixelData generate_audio_view_simple(std::vector<float>& mono, int rows, int cols);
PixelData generate_audio_view_maxed(std::vector<float>& mono, int rows, int cols);
PixelData generate_audio_view_amplitude_averaged(std::vector<float>& mono, int rows, int cols);

#endif