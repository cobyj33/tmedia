#ifndef TMEDIA_PALETTE_H
#define TMEDIA_PALETTE_H

#include "color.h"
#include <unordered_set>

typedef std::unordered_set<RGBColor, RGBColorHashFunction> Palette;

bool palette_equals(Palette& first, Palette& second);
Palette palette_color_cube(int side);
Palette palette_grayscale(int entry_count);

#endif