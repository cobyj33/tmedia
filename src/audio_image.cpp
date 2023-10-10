#include "audio_image.h"
#include <vector>
#include "audiobuffer.h"
#include "audio.h"
#include "canvas.h"

PixelData generate_audio_view(AudioBuffer& audio_buffer) {
  // std::vector<float> buffer_view = audio_buffer.peek_into(80);
  // std::vector<float> mono = audio_to_mono(buffer_view, audio_buffer.get_nb_channels());

  Canvas canvas(80, 24);
  for (int i = 0; i < 80; i++) {
    canvas.line(i, 10, i, 70, RGBColor::BLACK);
  }

  return canvas.get_image();
}
