#ifndef TMEDIA_TMCURSES_INTERNAL_H
#define TMEDIA_TMCURSES_INTERNAL_H

#ifndef TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION
#error "Cannot include internal tmcurses implementation details in public api. Include exposed functions through tmedia/tmcurses/tmcurses.h instead"
#endif

#include <tmedia/image/color.h>
#include <tmedia/tmcurses/tmcurses.h>
#include <string>

void tmcurses_init_color();
void tmcurses_uninit_color();

// https://no-color.org/
// Checks if the environment variable NO_COLOR is set and non-empty
bool no_color_is_set();

struct ColorPair {
  RGB24 fg;
  RGB24 bg;
  constexpr ColorPair() : fg(RGB24(0)), bg(RGB24(0)) { } 
  constexpr ColorPair(const RGB24 fg, const RGB24 bg) : fg(fg), bg(bg) {}
  constexpr ColorPair(const ColorPair& cp) : fg(cp.fg), bg(cp.bg) {}
  constexpr ColorPair(ColorPair&& cp) : fg(cp.fg), bg(cp.bg) {}
};

struct CursesColorPair {
  int fg;
  int bg;
  constexpr CursesColorPair() : fg(0), bg(0) { } 
  constexpr CursesColorPair(int fg, int bg) : fg(fg), bg(bg) {}
  constexpr CursesColorPair(const CursesColorPair& cp) : fg(cp.fg), bg(cp.bg) {}
  constexpr CursesColorPair(CursesColorPair&& cp) : fg(cp.fg), bg(cp.bg) {}
};

const char* tmcurses_color_palette_cstr(TMNCursesColorPalette colorPalette);

RGB24 tmcurses_get_color_number_content(curses_color_t);
ColorPair tmcurses_get_pair_number_content(curses_color_pair_t);

int tmcurses_init_rgb_color_palette();
int tmcurses_init_grayscale_color_palette();

void tmcurses_init_color_pairs();
void tmcurses_init_color_maps();

curses_color_t tmcurses_find_best_initialized_color_number(const RGB24&);
curses_color_pair_t tmcurses_find_best_initialized_color_pair(const RGB24&);

#endif