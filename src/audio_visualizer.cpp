#include "audio_visualizer.h"

#include "audio.h"
#include "canvas.h"
#include "pixeldata.h"
#include "wmath.h"

#include <memory>
#include <utility>
#include <vector>


std::vector<float> audio_float_buffer_to_normalized_mono(float* frames, int nb_frames, int nb_channels);

AmplitudeAbs::AmplitudeAbs() {}
PixelData AmplitudeAbs::visualize(float* frames, int nb_frames, int nb_channels, int width, int height) {
  const int rows = height;
  const int cols = width;
  std::vector<float> mono = audio_float_buffer_to_normalized_mono(frames, nb_frames, nb_channels);

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

std::unique_ptr<Visualizer> AmplitudeAbs::clone() {
  return std::move(std::make_unique<AmplitudeAbs>());
} 

AmplitudeSimple::AmplitudeSimple() {}
PixelData AmplitudeSimple::visualize(float* frames, int nb_frames, int nb_channels, int width, int height) {
  const int rows = height;
  const int cols = width;
  std::vector<float> mono = audio_float_buffer_to_normalized_mono(frames, nb_frames, nb_channels);

  const int middle_row = rows / 2;
  
  Canvas canvas(rows, cols);
  for (int col = 0; col < cols; col++) {
    const float sample = std::abs(mono[mono.size() * ((double)col / cols)]);
    const int line_size = rows * sample;
    canvas.line(clamp(middle_row - (line_size / 2), 0, rows - 1), col, clamp(middle_row + (line_size / 2), 0, rows - 1), col, RGBColor::WHITE);
  }

  return canvas.get_image();
}

std::unique_ptr<Visualizer> AmplitudeSimple::clone() {
  return std::move(std::make_unique<AmplitudeSimple>());
} 


AmplitudeMax::AmplitudeMax() {}
PixelData AmplitudeMax::visualize(float* frames, int nb_frames, int nb_channels, int width, int height) {
  const int rows = height;
  const int cols = width;
  std::vector<float> mono = audio_float_buffer_to_normalized_mono(frames, nb_frames, nb_channels);

  int middle_row = rows / 2;
  
  Canvas canvas(rows, cols);
  const double step_size = (double)mono.size() / (double)cols;
  for (int col = 0; col < cols; col++) {
    const std::size_t start = (std::size_t)((double)col * step_size);
    float sample = 0.0;
    for (std::size_t i = start; i < std::size_t(((double)col + 1.0) * step_size); i++) {
      sample = std::abs(std::max(mono[i], sample));
    }

    int line_size = rows * sample;
    canvas.line(clamp(middle_row - (line_size / 2), 0, rows - 1), col, clamp(middle_row + (line_size / 2), 0, rows - 1), col, RGBColor::WHITE);
  }

  return canvas.get_image();
}

std::unique_ptr<Visualizer> AmplitudeMax::clone() {
  return std::move(std::make_unique<AmplitudeMax>());
} 

std::vector<float> audio_float_buffer_to_normalized_mono(float* frames, int nb_frames, int nb_channels) {
  std::vector<float> audio_buffer_view(frames, frames + nb_frames * nb_channels);
  std::vector<float> mono = audio_to_mono(audio_buffer_view, nb_channels);
  audio_bound_volume(mono, 1, 1.0);
  return mono;
}