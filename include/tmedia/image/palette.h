#ifndef TMEDIA_PALETTE_H
#define TMEDIA_PALETTE_H

#include <tmedia/image/color.h>
#include <cstddef>
#include <unordered_set>
#include <string>

class RGBColorHashFunction {
public:
    // Ripped from http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf pg 666 sec 4.1 paragraph 2
    std::size_t operator()(RGB24 rgb) const {
      return (73856093UL * static_cast<std::size_t>(rgb.r)) ^
            (19349663UL * static_cast<std::size_t>(rgb.g)) ^
            (83492791UL * static_cast<std::size_t>(rgb.b));
    }
};

typedef std::unordered_set<RGB24, RGBColorHashFunction> Palette;

bool palette_equals(Palette& first, Palette& second);
Palette palette_color_cube(int side);
Palette palette_grayscale(int entry_count);
std::string palette_str(Palette& palette);

#endif
