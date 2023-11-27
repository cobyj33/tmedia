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