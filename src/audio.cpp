#include "audio.h"

#include "wmath.h"

#include <vector>
#include <cstddef>

std::vector<float> audio_to_mono(std::vector<float>& frames, int nb_channels) {
  std::vector<float> res;
  std::size_t nb_frames = frames.size() / nb_channels;
  for (std::size_t frame = 0; frame < nb_frames; frame++) {
    float mono = 0.0;
    for (int c = 0; c < nb_channels; c++) {
      mono += frames[frame * nb_channels + c];
    }
    res.push_back(mono);
  }

  return res;
}

void audio_to_mono_ipfb(float* frames, int nb_frames, int nb_channels) {
  int frame_offset = 0;
  for (int frame = 0; frame < nb_frames; frame++) {
    float mono = 0.0;
    frame_offset += nb_channels;
    for (int c = 0; c < nb_channels; c++) {
      mono += frames[frame_offset + c];
    }
    frames[frame] = mono;
  }
}

void audio_to_mono_ip(std::vector<float>& frames, int nb_channels) {
  std::size_t nb_frames = frames.size() / nb_channels;
  std::size_t frame_offset = 0;
  for (std::size_t frame = 0; frame < nb_frames; frame++) {
    float mono = 0.0;
    frame_offset += nb_channels;
    for (int c = 0; c < nb_channels; c++) {
      mono += frames[frame_offset + c];
    }
    frames[frame] = mono;
  }
  frames.resize(nb_frames);
}

void audio_bound_volume(std::vector<float>& frames, int nb_channels, float maxval) {
  #if 0
  for (int c = 0; c < nb_channels; c++) { // bound each channel individually
    float largest = 0.0;
    for (std::size_t s = c; s < frames.size(); s += nb_channels) {
      largest = max(frames[s], largest);
    }

    if (largest < maxval)
      continue;

    for (std::size_t s = c; s < frames.size(); s += nb_channels) {
      frames[s] *= maxval / largest;
    }
  }
  #endif
  float largest = 0.0;
  for (std::size_t s = 0; s < frames.size(); s++) {
    largest = max(frames[s], largest);
  }

  for (std::size_t s = 0; s < frames.size(); s++) {
    frames[s] *= maxval / largest;
  }

  (void)nb_channels;
}

void audio_bound_volume_fb(float* frames, int nb_frames, int nb_channels, float maxval) {
  int size = nb_frames * nb_channels;
  for (int c = 0; c < nb_channels; c++) { // bound each channel individually
    float largest = 0.0;
    for (int s = c; s < size; s += nb_channels) {
      largest = max(frames[s], largest);
    }

    if (largest < maxval)
      continue;

    for (int s = c; s < size; s += nb_channels) {
      frames[s] *= maxval / largest;
    }
  }
}

void audio_normalize(std::vector<float>& frames, int nb_channels) {
  for (int c = 0; c < nb_channels; c++) { // normalize each channel individually
    float largest = 0.0;
    for (std::size_t s = c; s < frames.size(); s += nb_channels) {
      largest = max(frames[s], largest);
    }

    if (largest == 0.0)
      continue;

    for (std::size_t s = c; s < frames.size(); s += nb_channels) {
      frames[s] /= largest;
    }
  }
}