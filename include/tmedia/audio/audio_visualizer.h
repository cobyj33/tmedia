#ifndef TMEDIA_AUDIO_IMAGE_H
#define TMEDIA_AUDIO_IMAGE_H

class PixelData;

void visualize(PixelData& dest, float* frames, int nb_frames, int nb_channels, int width, int height);

#endif