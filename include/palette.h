#ifndef TMEDIA_PALETTE_H
#define TMEDIA_PALETTE_H

#include "color.h"
#include <unordered_set>
#include <string>

typedef std::unordered_set<RGB24, RGBColorHashFunction> Palette;

bool palette_equals(Palette& first, Palette& second);
Palette palette_color_cube(int side);
Palette palette_grayscale(int entry_count);
std::string palette_str(Palette& palette);

#endif