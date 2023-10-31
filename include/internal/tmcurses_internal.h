#ifndef TMEDIA_TMCURSES_INTERNAL_H
#define TMEDIA_TMCURSES_INTERNAL_H

#ifndef TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION
#error "Cannot include internal tmcurses implementation details in public api. Include exposed functions through tmcurses.h instead"
#endif

#include "color.h"
#include "tmcurses.h"
#include <string>

void ncurses_init_color();
void ncurses_uninit_color();

struct ColorPair {
  RGBColor fg;
  RGBColor bg;
  ColorPair() : fg(RGBColor(0)), bg(RGBColor(0)) { } 
  ColorPair(RGBColor fg, RGBColor bg) : fg(fg), bg(bg) {}
  ColorPair(RGBColor& fg, RGBColor& bg) : fg(fg), bg(bg) {}
  ColorPair(const ColorPair& cp) : fg(cp.fg), bg(cp.bg) {}
};

struct CursesColorPair {
  int fg;
  int bg;
  CursesColorPair() : fg(0), bg(0) { } 
  CursesColorPair(int fg, int bg) : fg(fg), bg(bg) {}
  CursesColorPair(const CursesColorPair& cp) : fg(cp.fg), bg(cp.bg) {}
};

std::string ncurses_color_palette_string(TMNCursesColorPalette colorPalette);

RGBColor ncurses_get_color_number_content(curses_color_t);
ColorPair ncurses_get_pair_number_content(curses_color_pair_t);

int ncurses_init_rgb_color_palette();
int ncurses_init_grayscale_color_palette();

void ncurses_init_color_pairs();
void ncurses_init_color_maps();

curses_color_t ncurses_find_best_initialized_color_number(RGBColor&);
curses_color_pair_t ncurses_find_best_initialized_color_pair(RGBColor&);

#endif