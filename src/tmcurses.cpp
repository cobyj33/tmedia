#include "tmcurses.h"

#include "color.h"
#include "wmath.h"
#include "palette.h"
#define TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION
#include "internal/tmcurses_internal.h"
#undef TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION

#include <cmath>
#include <vector>
#include <stdexcept>
#include <string>

#include <fmt/format.h>
#include "funcmac.h"

extern "C" {
#include <curses.h>
}

bool ncurses_initialized = false;

void ncurses_init() {
  if (ncurses_initialized) return;
  ncurses_initialized = true;
  initscr();
  savetty();
  ncurses_init_color();
  ncurses_set_color_palette(TMNCursesColorPalette::RGB);
  cbreak();
  noecho();
  nodelay(stdscr, true);
  timeout(5);
  keypad(stdscr, true);
  intrflush(stdscr, false); // maybe set to true?
  idcok(stdscr, true);
  idlok(stdscr, true);
  leaveok(stdscr, true);
  nonl();
  curs_set(0);
}

bool ncurses_is_initialized() {
  return ncurses_initialized;
}

void ncurses_uninit() {
  if (!ncurses_initialized) return;
  ncurses_initialized = false;
  erase();
  refresh();
  ncurses_uninit_color();
  keypad(stdscr, false);
  nocbreak();
  echo();
  resetty();
  endwin();
}


static constexpr int COLOR_MAP_SIDE = 7;
static constexpr int MAX_TERMINAL_COLORS = 256;
static constexpr int MAX_TERMINAL_COLOR_PAIRS = 256;
static constexpr int DEFAULT_TERMINAL_COLOR_COUNT = 8;

/**
 * Some terminals (namely Windows Terminal) dont want to return to their
 * normal color scheme on ncurses return.
 * Luckily, Windows Terminal only uses 16 colors in their internal color scheme, 
 * so we just ignore them and offset the internal color palette by 16.
 * It is important to note that this could be larger than COLORS if COLORS < 16.
 * 
 * While this usually reduces our 256 color palette to 240 colors, the 7x7x7 
 * color_map and color_pairs_map can only hold 216 colors anyway so it isn't
 * much of a problem
 * 
 * Note that curses color pairs are not offset like colors are. curses color
 * pairs fill from 0 to COLOR_PAIRS - 1 as expected.
*/
static constexpr int COLOR_PALETTE_START = 16;

/**
 * Its important to not mix the Color Palette's colors and the terminals standard
 * default colors not only for not messing with a terminals default 16 color palette,
 * but also because in color palettes like grayscale, we wouldn't want a stray
 * COLOR_RED or COLOR_GREEN color pair returning from get_ncurses_color_pair 
*/

bool curses_colors_initialized = false;

/**
 * @brief A RGB mapping of NCURSES color pairs to a 3D Discrete Color space, where each side of the color space is COLOR_MAP_SIDE steps long
 * To index into the map, take each channel's ratio against 255 and multiply it against COLOR_MAP_SIDE (ex: (int)(red / 255.0 * COLOR_MAP_SIZE))
 * Additionally, the backgrounds of all colors should be a certain color, while the foreground is that color's complementary value
 */
curses_color_pair_t color_pairs_map[COLOR_MAP_SIDE][COLOR_MAP_SIDE][COLOR_MAP_SIDE];
curses_color_t color_map[COLOR_MAP_SIDE][COLOR_MAP_SIDE][COLOR_MAP_SIDE];

int available_color_palette_colors = 0;
int available_color_palette_color_pairs = 0;

// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// ----------------------- PUBLIC API ---------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------

void ncurses_init_color() {
  if (!has_colors()) return;
  if (!can_change_color()) return;
  if (curses_colors_initialized) return;

  start_color();
  if (COLORS <= COLOR_PALETTE_START) return;

  use_default_colors();
  
  available_color_palette_colors = 0;
  available_color_palette_color_pairs = 0;

  ncurses_init_color_pairs();
  ncurses_init_color_maps();
  curses_colors_initialized = true;
}

void ncurses_uninit_color() {
  available_color_palette_colors = 0;
  available_color_palette_color_pairs = 0;
  curses_colors_initialized = false;
}

void ncurses_set_color_palette(TMNCursesColorPalette colorPalette) {
  if (!curses_colors_initialized) return;

  available_color_palette_colors = 0;
  switch (colorPalette) {
    case TMNCursesColorPalette::RGB: available_color_palette_colors = ncurses_init_rgb_color_palette(); break;
    case TMNCursesColorPalette::GRAYSCALE: available_color_palette_colors = ncurses_init_grayscale_color_palette(); break;
  }

  ncurses_init_color_pairs();
  ncurses_init_color_maps();
}

void ncurses_set_color_palette_custom(const Palette& colorPalette) {
  if (!curses_colors_initialized) return;
  const short MAX_COLORS = std::min(COLORS, MAX_TERMINAL_COLORS);
  const short CHANGEABLE_COLORS = MAX_COLORS - COLOR_PALETTE_START;
  if (CHANGEABLE_COLORS <= 0) return;

  available_color_palette_colors = 0;
  for (RGB24 color : colorPalette) {
    init_color(available_color_palette_colors++ + COLOR_PALETTE_START, 
    static_cast<short>(color.r) * 1000 / 255,
    static_cast<short>(color.g) * 1000 / 255,
    static_cast<short>(color.b) * 1000 / 255);
    if (available_color_palette_colors >= CHANGEABLE_COLORS) break;
  }

  ncurses_init_color_pairs();
  ncurses_init_color_maps();

}



curses_color_t get_closest_ncurses_color(const RGB24& input) {
  if (!curses_colors_initialized) return 0; // just return a default 0 to no-op

  return color_map[static_cast<int>(input.r) * (COLOR_MAP_SIDE - 1) / 255]
                  [static_cast<int>(input.g) * (COLOR_MAP_SIDE - 1) / 255]
                  [static_cast<int>(input.b) * (COLOR_MAP_SIDE - 1) / 255];
}

curses_color_pair_t get_closest_ncurses_color_pair(const RGB24& input) {
  if (!curses_colors_initialized) return 0; // just return a default 0 to no-op

  return color_pairs_map[static_cast<int>(input.r) * (COLOR_MAP_SIDE - 1) / 255]
                      [static_cast<int>(input.g) * (COLOR_MAP_SIDE - 1) / 255]
                      [static_cast<int>(input.b) * (COLOR_MAP_SIDE - 1) / 255];
}

// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// ------------------------- BACKEND ----------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------

std::string ncurses_color_palette_string(TMNCursesColorPalette colorPalette) {
  switch (colorPalette) {
    case TMNCursesColorPalette::RGB: return "rgb";
    case TMNCursesColorPalette::GRAYSCALE: return "grayscale";
  }
  return "unknown";
}

RGB24 ncurses_get_color_number_content(curses_color_t color) {
  short r, g, b;
  color_content(color, &r, &g, &b);
  return RGB24(r * 255 / 1000, g * 255 / 1000, b * 255 / 1000);
}

ColorPair ncurses_get_pair_number_content(curses_color_pair_t pair) {
  short fgi, bgi;
  pair_content(pair, &fgi, &bgi);
  return ColorPair(ncurses_get_color_number_content(fgi), ncurses_get_color_number_content(bgi));
}

int ncurses_init_rgb_color_palette() {
  const double NCURSES_COLOR_COMPONENT_MAX = 1000.;
  const short MAX_COLORS = std::min(COLORS, MAX_TERMINAL_COLORS);
  const short CHANGEABLE_COLORS = MAX_COLORS - COLOR_PALETTE_START;
  if (CHANGEABLE_COLORS <= 0) return 0;

  const double BOX_SIZE = NCURSES_COLOR_COMPONENT_MAX / std::cbrt(CHANGEABLE_COLORS);

  int color_index = COLOR_PALETTE_START;
  for (double r = 0; r < NCURSES_COLOR_COMPONENT_MAX; r += BOX_SIZE)
    for (double g = 0; g < NCURSES_COLOR_COMPONENT_MAX; g += BOX_SIZE)
      for (double b = 0; b < NCURSES_COLOR_COMPONENT_MAX; b += BOX_SIZE)
        init_color(color_index++, static_cast<short>(r), static_cast<short>(g), static_cast<short>(b));

  return color_index;
}


int ncurses_init_grayscale_color_palette() {
  const short NCURSES_COLOR_COMPONENT_MAX = 1000;
  const short MAX_COLORS = std::min(COLORS, MAX_TERMINAL_COLORS);
  const short CHANGEABLE_COLORS = MAX_COLORS - COLOR_PALETTE_START;
  if (CHANGEABLE_COLORS <= 0) return 0;

  for (short i = 0; i < CHANGEABLE_COLORS; i++) {
    const short grayscale_value = i * NCURSES_COLOR_COMPONENT_MAX / CHANGEABLE_COLORS;
    init_color(i + COLOR_PALETTE_START, grayscale_value, grayscale_value, grayscale_value);
  }

  return MAX_COLORS;
}

void ncurses_init_color_pairs() {
  const int COLOR_PAIRS_TO_INIT = std::min(COLOR_PAIRS, available_color_palette_colors);
  for (int i = 0; i < COLOR_PAIRS_TO_INIT; i++) {
    curses_color_pair_t color_pair_index = i;
    curses_color_t color_index = i + COLOR_PALETTE_START;
    RGB24 color = ncurses_get_color_number_content(color_index);
    RGB24 complementary = color.get_comp();
    init_pair(color_pair_index, ncurses_find_best_initialized_color_number(complementary), color_index);
  }

  available_color_palette_color_pairs = COLOR_PAIRS_TO_INIT;
}

void ncurses_init_color_maps() {
  for (int r = 0; r < COLOR_MAP_SIDE; r++) {
    for (int g = 0; g < COLOR_MAP_SIDE; g++) {
      for (int b = 0; b < COLOR_MAP_SIDE; b++) {
        RGB24 color( r * 255 / (COLOR_MAP_SIDE - 1), g * 255 / (COLOR_MAP_SIDE - 1), b * 255 / (COLOR_MAP_SIDE - 1) );
        color_map[r][g][b] = ncurses_find_best_initialized_color_number(color);
        color_pairs_map[r][g][b] = ncurses_find_best_initialized_color_pair(color);
      }
    }
  }
}

curses_color_t ncurses_find_best_initialized_color_number(RGB24& input) {
  curses_color_t best_color_index = -1;
  double best_distance = (double)INT32_MAX;
  for (int i = 0; i < available_color_palette_colors; i++) {
    curses_color_t color_index = i + COLOR_PALETTE_START;
    RGB24 current_color = ncurses_get_color_number_content(color_index);
    double distance = current_color.dis_sq(input);
    if (distance < best_distance) {
      best_color_index = color_index;
      best_distance = distance;
    }
  }

  return best_color_index;
}

curses_color_pair_t ncurses_find_best_initialized_color_pair(RGB24& input) {
  curses_color_pair_t best_pair_index = -1;
  double best_distance = (double)INT32_MAX;
  for (int i = 0; i < available_color_palette_color_pairs; i++) {
    curses_color_pair_t color_pair_index = i;
    ColorPair color_pair = ncurses_get_pair_number_content(color_pair_index);
    const double distance = color_pair.bg.dis_sq(input);
    if (distance < best_distance) {
      best_pair_index = color_pair_index;
      best_distance = distance;
    }
  }

  return best_pair_index;
}
