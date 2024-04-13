#include <tmedia/image/palette.h>

#include <cstdint>
#include <sstream>

bool palette_equals(Palette& first, Palette& second) {
  if (first.size() != second.size()) {
    return false;
  }

  for (const RGB24& color : first) {
    if (second.count(color) != 1) {
      return false;
    }
  }

  return true;
}

std::string palette_str(Palette& palette) {
  if (palette.size() == 0) return "{}";
  std::stringstream buf;
  buf << "{\n";
  for (const RGB24& color : palette) {
    buf << "\t{" << "r: " << color.r << ", g: " << color.g << " b: " <<
      color.b << "}\n";
  }
  buf << "}";
  return buf.str();
}

Palette palette_color_cube(int side) {
  Palette palette;
  const double BOX_SIZE = 255.0 / side;

  for (double r = 0; r < 255.0; r += BOX_SIZE)
    for (double g = 0; g < 255.0; g += BOX_SIZE)
      for (double b = 0; b < 255.0; b += BOX_SIZE)
        palette.insert(RGB24(static_cast<std::uint8_t>(r),
                                static_cast<std::uint8_t>(g),
                                static_cast<std::uint8_t>(b)));

  return palette;
}

Palette palette_grayscale(int entry_count) {
  Palette palette;

  for (int i = 0; i < entry_count; i++) {
    palette.insert(RGB24(static_cast<uint8_t>(i * 255 / entry_count)));
  }

  return palette;
}