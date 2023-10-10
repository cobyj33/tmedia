#ifndef ASCII_VIDEO_AUDIO_IMAGE_H
#define ASCII_VIDEO_AUDIO_IMAGE_H

#include "audiobuffer.h"
#include "pixeldata.h"
#include <cstddef>

PixelData generate_audio_view(AudioBuffer& audio_buffer);

#endif