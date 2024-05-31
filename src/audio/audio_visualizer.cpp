#include <tmedia/audio/audio_visualizer.h>

#include <tmedia/audio/audio.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/util/wmath.h>

#include <memory>
#include <utility>
#include <vector>
#include <cassert>

void visualize(PixelData& dest, float* frames, int nb_frames, int nb_channels, int width, int height) {
  assert(width > 0);
  assert(height > 0);

  const std::size_t nb_samples = nb_frames * nb_channels;
  const int middle_row = height / 2;
  pixdata_initzero(dest, width, height);
  
  const float step_size = nb_samples / static_cast<float>(width);
  for (int col = 0; col < width; col++) {
    const std::size_t start = static_cast<std::size_t>(static_cast<float>(col) * step_size);
    float sample = 0.0;
    float count = 0.0;
    const std::size_t step_end = static_cast<std::size_t>(static_cast<float>(col + 1.0) * step_size);
    for (std::size_t i = start; i < step_end; i++) {
      sample += std::abs(frames[i]);
      count++;
    }

    if (count != 0)
      sample /= count;
    const int line_size = height * sample;
    vertline(dest, clamp(middle_row - (line_size / 2), 0, height - 1), clamp(middle_row + (line_size / 2), 0, height - 1), col, RGB24::WHITE);
  }
}