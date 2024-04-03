#ifndef TMEDIA_AUDIO_IMAGE_H
#define TMEDIA_AUDIO_IMAGE_H

#include <vector>
#include <memory>

class PixelData;

/**
 * General strategy class for generating audio data from any inputted audio frames
*/
class Visualizer {
  public:
    virtual PixelData visualize(float* frames, int nb_frames, int nb_channels, int width, int height) = 0;
    virtual ~Visualizer() {}
    virtual std::unique_ptr<Visualizer> clone() = 0;
};

class AmplitudeAbs : public Visualizer {
  public:
    AmplitudeAbs();
    PixelData visualize(float* frames, int nb_frames, int nb_channels, int width, int height);
    virtual std::unique_ptr<Visualizer> clone();
};

class AmplitudeMax : public Visualizer {
  public:
    AmplitudeMax();
    PixelData visualize(float* frames, int nb_frames, int nb_channels, int width, int height);
    virtual std::unique_ptr<Visualizer> clone();
};

class AmplitudeSimple : public Visualizer {
  public:
    AmplitudeSimple();
    PixelData visualize(float* frames, int nb_frames, int nb_channels, int width, int height);
    virtual std::unique_ptr<Visualizer> clone();
};


PixelData generate_audio_view_simple(std::vector<float>& mono, int rows, int cols);
PixelData generate_audio_view_maxed(std::vector<float>& mono, int rows, int cols);

#endif