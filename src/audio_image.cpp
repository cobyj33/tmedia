#include "audio_image.h"
#include "audiobuffer.h"
#include "audio.h"
#include "canvas.h"
#include "wmath.h"

#include <vector>
#include <algorithm>

PixelData generate_audio_view(std::vector<float>& mono, int rows, int cols) {
  int middle_row = rows / 2;
  
  Canvas canvas(rows, cols); // 24 rows by 80 columns

  for (int col = 0; col < cols; col++) {
    float sample = mono[(std::size_t)(mono.size() * ((double)col / cols))];
    int line_size = rows * sample;
    canvas.line(clamp(middle_row - (line_size / 2), 0, rows - 1), col, clamp(middle_row + (line_size / 2), 0, rows - 1), col, RGBColor::WHITE);
  }

  return canvas.get_image();
}
