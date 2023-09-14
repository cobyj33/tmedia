#ifndef ASCII_VIDEO_TERMCOLOR_INTERNAL_H
#define ASCII_VIDEO_TERMCOLOR_INTERNAL_H

#ifndef ASCII_VIDEO_TERMCOLOR_INTERNAL_IMPLEMENTATION
#error "Cannot include internal termcolor implementation details in public api. Include exposed functions through termcolor.h instead"
#endif

#include "color.h"
#include <string>

typedef short curses_color_pair_t;
typedef short curses_color_t;

struct ColorPair {
  RGBColor fg;
  RGBColor bg;
  ColorPair() : fg(RGBColor(0)), bg(RGBColor(0)) { } 
  ColorPair(RGBColor fg, RGBColor bg) : fg(fg), bg(bg) {}
  ColorPair(RGBColor& fg, RGBColor& bg) : fg(fg), bg(bg) {}
  ColorPair(const ColorPair& cp) : fg(cp.fg), bg(cp.bg) {}
};

std::string ncurses_color_palette_string(AVNCursesColorPalette colorPalette);

RGBColor ncurses_get_color_number_content(curses_color_t);
ColorPair ncurses_get_pair_number_content(curses_color_pair_t);

int ncurses_init_rgb_color_palette();
int ncurses_init_grayscale_color_palette();

curses_color_t ncurses_find_best_initialized_color_number(RGBColor&);
curses_color_pair_t ncurses_find_best_initialized_color_pair(RGBColor&);

#endif