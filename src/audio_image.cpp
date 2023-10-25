#include "audio_image.h"

#include "canvas.h"
#include "wmath.h"

#include <vector>

PixelData generate_audio_view_simple(std::vector<float>& mono, int rows, int cols) {
  int middle_row = rows / 2;
  
  Canvas canvas(rows, cols);
  for (int col = 0; col < cols; col++) {
    float sample = std::abs(mono[mono.size() * ((double)col / cols)]);
    int line_size = rows * sample;
    canvas.line(clamp(middle_row - (line_size / 2), 0, rows - 1), col, clamp(middle_row + (line_size / 2), 0, rows - 1), col, RGBColor::WHITE);
  }

  return canvas.get_image();
}

PixelData generate_audio_view_amplitude_averaged(std::vector<float>& mono, int rows, int cols) {
  int middle_row = rows / 2;
  
  Canvas canvas(rows, cols);
  const double step_size = (double)mono.size() / (double)cols;
  for (int col = 0; col < cols; col++) {
    std::size_t start = (std::size_t)((double)col * step_size);
    float sample = 0.0;
    int count = 0;
    for (std::size_t i = start; i < std::size_t(((double)col + 1.0) * step_size); i++) {
      sample += std::abs(mono[i]);
      count++;
    }

    if (count != 0)
      sample /= count;

    int line_size = rows * sample;
    canvas.line(clamp(middle_row - (line_size / 2), 0, rows - 1), col, clamp(middle_row + (line_size / 2), 0, rows - 1), col, RGBColor::WHITE);
  }

  return canvas.get_image();
}

PixelData generate_audio_view_maxed(std::vector<float>& mono, int rows, int cols) {
  int middle_row = rows / 2;
  
  Canvas canvas(rows, cols);
  const double step_size = (double)mono.size() / (double)cols;
  for (int col = 0; col < cols; col++) {
    std::size_t start = (std::size_t)((double)col * step_size);
    float sample = 0.0;
    for (std::size_t i = start; i < std::size_t(((double)col + 1.0) * step_size); i++) {
      sample = std::abs(std::max(mono[i], sample));
    }

    int line_size = rows * sample;
    canvas.line(clamp(middle_row - (line_size / 2), 0, rows - 1), col, clamp(middle_row + (line_size / 2), 0, rows - 1), col, RGBColor::WHITE);
  }

  return canvas.get_image();
}
