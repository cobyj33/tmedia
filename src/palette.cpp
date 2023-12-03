#include "palette.h"

bool palette_equals(Palette& first, Palette& second) {
  if (first.size() != second.size()) {
    return false;
  }

  for (const RGBColor& color : first) {
    if (second.count(color) != 1) {
      return false;
    }
  }

  return true;
}

Palette palette_color_cube(int side) {
  Palette palette;
  const double BOX_SIZE = 255.0 / side;

  for (double r = 0; r < 255.0; r += BOX_SIZE)
    for (double g = 0; g < 255.0; g += BOX_SIZE)
      for (double b = 0; b < 255.0; b += BOX_SIZE)
        palette.insert(RGBColor((int)r, (int)g, (int)b));

  return palette;
}

Palette palette_grayscale(int entry_count) {
  Palette palette;

  for (short i = 0; i < entry_count; i++) {
    const short grayscale_value = i * 255 / entry_count;
    palette.insert(RGBColor(grayscale_value));
  }

  return palette;
}