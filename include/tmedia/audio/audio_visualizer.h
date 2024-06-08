#ifndef TMEDIA_AUDIO_IMAGE_H
#define TMEDIA_AUDIO_IMAGE_H

class PixelData;

/**
 * 
 * @param dest The PixelData instance where the visualized frames are written
 * @param width width must be greater than or equal to zero
 * @param height height must be greater than or equal to zero
 */
void visualize(PixelData& dest, float* frames, int nb_frames, int nb_channels, int width, int height);

#endif