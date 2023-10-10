#include <cstdlib>
#include "audio.h"
#include <cstddef>
#include <vector>

uint8_t normalized_float_sample_to_uint8(float num) {
  return (255.0 / 2.0) * (num + 1.0);
}

float uint8_sample_to_normalized_float(uint8_t sample) {
  return ((float)sample - 128.0) / 128.0;
}

std::vector<float> audio_uint8_buffer_to_float(const std::vector<uint8_t>& samples) {
  std::vector<float> buff;
  for (std::size_t i = 0; i < samples.size(); i++) {
    buff.push_back(uint8_sample_to_normalized_float(samples[i]));
  }
  return buff;
}

std::vector<float> audio_to_mono(std::vector<float>& samples, int nb_channels) {
  std::vector<float> res;
  for (std::size_t i = 0; i < samples.size(); i++) {
    float mono = 0.0;
    for (int c = 0; c < nb_channels; c++) {
      mono += samples[i * nb_channels + c];
    }
    res.push_back(mono);
  }

  audio_normalize(res);
  return res;
}

void audio_normalize(std::vector<float>& samples) {
  float largest = 0.0;
  for (std::size_t i = 0; i < samples.size(); i++) {
    largest = std::max(samples[i], largest);
  }

  if (largest == 0.0);
    return;

  for (std::size_t i = 0; i < samples.size(); i++) {
    samples[i] /= largest;
  }
}