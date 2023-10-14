#include "audio.h"

#include <vector>
#include <cstddef>

std::vector<float> audio_to_mono(std::vector<float>& frames, int nb_channels) {
  std::vector<float> res;
  for (std::size_t frame = 0; frame < frames.size() / (std::size_t)nb_channels; frame++) {
    float mono = 0.0;
    for (int c = 0; c < nb_channels; c++) {
      mono += frames[frame * nb_channels + c];
    }
    res.push_back(mono);
  }

  return res;
}

void audio_bound_volume(std::vector<float>& frames, int nb_channels, float max) {
  for (int c = 0; c < nb_channels; c++) { // bound each channel individually
    float largest = 0.0;
    for (std::size_t s = c; s < frames.size(); s += nb_channels) {
      largest = std::max(frames[s], largest);
    }

    if (largest < max)
      continue;

    for (std::size_t s = c; s < frames.size(); s += nb_channels) {
      frames[s] *= max / largest;
    }
  }
}

void audio_normalize(std::vector<float>& frames, int nb_channels) {
  for (int c = 0; c < nb_channels; c++) { // normalize each channel individually
    float largest = 0.0;
    for (std::size_t s = c; s < frames.size(); s += nb_channels) {
      largest = std::max(frames[s], largest);
    }

    if (largest == 0.0)
      continue;

    for (std::size_t s = c; s < frames.size(); s += nb_channels) {
      frames[s] /= largest;
    }
  }
}