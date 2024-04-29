#include <tmedia/audio/audio_visualizer.h>

#include <tmedia/audio/audio.h>
#include <tmedia/image/canvas.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/util/wmath.h>

#include <memory>
#include <utility>
#include <vector>

PixelData visualize(float* frames, int nb_frames, int nb_channels, int width, int height) {
  const int rows = height;
  const int cols = width;
  const std::size_t nb_samples = nb_frames * nb_channels;
  const int middle_row = rows / 2;
  
  Canvas canvas(rows, cols);
  const float step_size = nb_samples / static_cast<float>(cols);
  for (int col = 0; col < cols; col++) {
    std::size_t start = static_cast<std::size_t>(static_cast<float>(col) * step_size);
    float sample = 0.0;
    float count = 0.0;
    const std::size_t step_end = static_cast<std::size_t>(static_cast<float>(col + 1.0) * step_size);
    for (std::size_t i = start; i < step_end; i++) {
      sample += std::abs(frames[i]);
      count++;
    }

    if (count != 0)
      sample /= count;
    const int line_size = rows * sample;
    canvas.vertline(clamp(middle_row - (line_size / 2), 0, rows - 1), clamp(middle_row + (line_size / 2), 0, rows - 1), col, RGB24::WHITE);
  }

  return canvas.get_image();
}