#include "audio.h"

#include <vector>
#include <cstddef>

std::vector<float> audio_to_mono(std::vector<float>& samples, int nb_channels) {
  std::vector<float> res;
  for (std::size_t frame = 0; frame < samples.size() / (std::size_t)nb_channels; frame++) {
    float mono = 0.0;
    for (int c = 0; c < nb_channels; c++) {
      mono += samples[frame * nb_channels + c];
    }
    res.push_back(mono);
  }

  return res;
}

void audio_bound_volume(std::vector<float>& samples, float max) {
  float largest = 0.0;
  for (std::size_t i = 0; i < samples.size(); i++) {
    largest = std::max(samples[i], largest);
  }

  if (largest < max)
    return;

  for (std::size_t i = 0; i < samples.size(); i++) {
    samples[i] *= max / largest;
  }
}

void audio_normalize(std::vector<float>& samples) {
  float largest = 0.0;
  for (std::size_t i = 0; i < samples.size(); i++) {
    largest = std::max(samples[i], largest);
  }

  if (largest == 0.0)
    return;

  for (std::size_t i = 0; i < samples.size(); i++) {
    samples[i] /= largest;
  }
}