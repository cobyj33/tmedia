#include <tmedia/audio/audio_visualizer.h>

#include <tmedia/audio/audio.h>
#include <tmedia/image/canvas.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/util/wmath.h>

#include <memory>
#include <utility>
#include <vector>



std::vector<float> audio_float_buffer_to_normalized_mono(float* frames, int nb_frames, int nb_channels);
void afb_to_nmbuf(float* frames, int nb_frames, int nb_channels);

AmplitudeAbs::AmplitudeAbs() {}
PixelData AmplitudeAbs::visualize(float* frames, int nb_frames, int nb_channels, int width, int height) {
  const int rows = height;
  const int cols = width;
  const std::size_t nb_samples = nb_frames * nb_channels;
  const int middle_row = rows / 2;
  
  Canvas canvas(rows, cols);
  const float step_size = nb_samples / (float)cols;
  for (int col = 0; col < cols; col++) {
    std::size_t start = (std::size_t)((float)col * step_size);
    float sample = 0.0;
    float count = 0.0; // prevent divide by 0
    const std::size_t step_end = ((float)col + 1.0) * step_size;
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

std::unique_ptr<Visualizer> AmplitudeAbs::clone() {
  return std::make_unique<AmplitudeAbs>();
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
    canvas.vertline(clamp(middle_row - (line_size / 2), 0, rows - 1), clamp(middle_row + (line_size / 2), 0, rows - 1), col, RGB24::WHITE);
  }

  return canvas.get_image();
}

std::unique_ptr<Visualizer> AmplitudeSimple::clone() {
  return std::make_unique<AmplitudeSimple>();
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
    canvas.vertline(clamp(middle_row - (line_size / 2), 0, rows - 1), clamp(middle_row + (line_size / 2), 0, rows - 1), col, RGB24::WHITE);
  }

  return canvas.get_image();
}

std::unique_ptr<Visualizer> AmplitudeMax::clone() {
  return std::make_unique<AmplitudeMax>();
} 

std::vector<float> audio_float_buffer_to_normalized_mono(float* frames, int nb_frames, int nb_channels) {
  std::vector<float> buf(frames, frames + (nb_frames * nb_channels));
  audio_to_mono_ip(buf, nb_channels);
  // audio should already be bounded to -1.0 to 1.0
  return buf;
}

void afb_to_nmbuf(float* frames, int nb_frames, int nb_channels) {
  audio_to_mono_ipfb(frames, nb_frames, nb_channels);
  audio_bound_volume_fb(frames, nb_frames, 1, 1.0);
}