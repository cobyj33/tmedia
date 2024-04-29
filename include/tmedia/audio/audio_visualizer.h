#ifndef TMEDIA_AUDIO_IMAGE_H
#define TMEDIA_AUDIO_IMAGE_H

#include <vector>
#include <memory>

class PixelData;

PixelData visualize(float* frames, int nb_frames, int nb_channels, int width, int height);

#endif