#ifndef ASCII_VIDEO_AUDIO_IMAGE_H
#define ASCII_VIDEO_AUDIO_IMAGE_H

#include "pixeldata.h"
#include <cstddef>

PixelData generate_audio_view(std::vector<float>& mono, int rows, int cols);

#endif