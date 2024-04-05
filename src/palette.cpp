#include "palette.h"

#include <cstdint>

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
        palette.insert(RGBColor(static_cast<std::uint8_t>(r),
                                static_cast<std::uint8_t>(g),
                                static_cast<std::uint8_t>(b)));

  return palette;
}

Palette palette_grayscale(int entry_count) {
  Palette palette;

  for (int i = 0; i < entry_count; i++) {
    palette.insert(RGBColor(static_cast<uint8_t>(i * 255 / entry_count)));
  }

  return palette;
}