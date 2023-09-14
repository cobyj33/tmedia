
#include <stdexcept>
#include <cmath>
#include <string>

#define ASCII_VIDEO_TERMCOLOR_INTERNAL_IMPLEMENTATION
#include "color.h"
#include "termcolor.h"
#include "internal/termcolor_internal.h"
#include "wmath.h"

extern "C" {
#include <curses.h>
}

const int GRAYSCALE_MAP_SIZE = 256;
const int COLOR_MAP_SIDE = 7;
const int MAX_TERMINAL_COLORS = 256;
const int DEFAULT_TERMINAL_COLOR_COUNT = 8;

bool curses_color_maps_initialized = false;
bool curses_colors_initialized = false;


RGBColor terminal_default_colors[MAX_TERMINAL_COLORS];
ColorPair terminal_default_color_pairs[MAX_TERMINAL_COLORS];

/**
 * @brief A RGB mapping of NCURSES color pairs to a 3D Discrete Color space, where each side of the color space is COLOR_MAP_SIDE steps long
 * To index into the map, take each channel's ratio against 255 and multiply it against COLOR_MAP_SIDE (ex: (int)(red / 255.0 * COLOR_MAP_SIZE))
 * Additionally, the backgrounds of all colors should be a certain color, while the foreground is that color's complementary value
 */
curses_color_pair_t color_pairs_map[COLOR_MAP_SIDE][COLOR_MAP_SIDE][COLOR_MAP_SIDE];
curses_color_t color_map[COLOR_MAP_SIDE][COLOR_MAP_SIDE][COLOR_MAP_SIDE];

int available_colors = 0;

int available_color_pairs = 0;

// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// ----------------------- PUBLIC API ---------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------

void ncurses_init_color() {
  if (!has_colors())
    throw std::runtime_error("Cannot initialize ncurses color");

  start_color();
  use_default_colors();
  curses_colors_initialized = true;
  curses_color_maps_initialized = false;
}

void ncurses_uninit_color() {
  start_color();
  curses_colors_initialized = false;
  curses_color_maps_initialized = false;
}

void ncurses_set_color_palette(AVNCursesColorPalette colorPalette) {
  if (!can_change_color()) {
    throw std::runtime_error("Cannot initialize ncurses color palette, The current terminal does not support changing colors");
  }

  switch (colorPalette) {
    case AVNCursesColorPalette::RGB: available_colors = ncurses_init_rgb_color_palette(); break;
    case AVNCursesColorPalette::GRAYSCALE: available_colors = ncurses_init_grayscale_color_palette(); break;
  }

  for (int i = 0; i < available_colors; i++) {
    RGBColor color = ncurses_get_color_number_content(i);
    RGBColor complementary = color.get_complementary();
    init_pair(i, get_closest_ncurses_color(complementary), i);
  }
  available_color_pairs = available_colors;

  for (int r = 0; r < COLOR_MAP_SIDE; r++) {
    for (int g = 0; g < COLOR_MAP_SIDE; g++) {
      for (int b = 0; b < COLOR_MAP_SIDE; b++) {
        RGBColor color( r * 255 / (COLOR_MAP_SIDE - 1), g * 255 / (COLOR_MAP_SIDE - 1), b * 255 / (COLOR_MAP_SIDE - 1) );
        color_map[r][g][b] = ncurses_find_best_initialized_color_number(color);
        color_pairs_map[r][g][b] = ncurses_find_best_initialized_color_pair(color);
      }
    }
  }

  curses_color_maps_initialized = true;
}

int get_closest_ncurses_color(const RGBColor& input) {
  if (!has_colors()) {
    throw std::runtime_error("Cannot find closest ncurses color pair, terminal does not support colors");
  }

  return color_map[input.red * (COLOR_MAP_SIDE - 1) / 255][input.green * (COLOR_MAP_SIDE - 1) / 255][input.blue * (COLOR_MAP_SIDE - 1) / 255];
}

int get_closest_ncurses_color_pair(const RGBColor& input) {
  if (!has_colors()) {
    throw std::runtime_error("Cannot find closest ncurses color pair, terminal does not support colors");
  }

  return color_pairs_map[input.red * (COLOR_MAP_SIDE - 1) / 255][input.green * (COLOR_MAP_SIDE - 1) / 255][input.blue * (COLOR_MAP_SIDE - 1) / 255];
}

// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// ------------------------- BACKEND ----------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------

std::string ncurses_color_palette_string(AVNCursesColorPalette colorPalette) {
  switch (colorPalette) {
    case AVNCursesColorPalette::RGB: return "rgb";
    case AVNCursesColorPalette::GRAYSCALE: return "grayscale";
  }
  return "unknown";
}

RGBColor ncurses_get_color_number_content(curses_color_t color) {
  short r, g, b;
  color_content(color, &r, &g, &b);
  return RGBColor((int)r * 255 / 1000, (int)g * 255 / 1000, (int)b * 255 / 1000);
}

ColorPair ncurses_get_pair_number_content(curses_color_pair_t pair) {
  short fgi, bgi;
  pair_content(pair, &fgi, &bgi);
  return ColorPair(ncurses_get_color_number_content(fgi), ncurses_get_color_number_content(bgi));
}



int ncurses_init_rgb_color_palette() {
  const short NCURSES_COLOR_COMPONENT_MAX = 1000;
  const short COLORS_AVAILABLE = std::min(COLORS, MAX_TERMINAL_COLORS);
  const double BOX_SIZE = NCURSES_COLOR_COMPONENT_MAX / std::cbrt(COLORS_AVAILABLE);

  int color_index = 0;

  for (double r = 0; r < (double)NCURSES_COLOR_COMPONENT_MAX; r += BOX_SIZE)
    for (double g = 0; g < (double)NCURSES_COLOR_COMPONENT_MAX; g += BOX_SIZE)
      for (double b = 0; b < (double)NCURSES_COLOR_COMPONENT_MAX; b += BOX_SIZE)
        init_color(color_index++, (short)r, (short)g, (short)b);

  return color_index;
}

int ncurses_init_grayscale_color_palette() {
  const short NCURSES_COLOR_COMPONENT_MAX = 1000;
  const short COLORS_AVAILABLE = std::min(COLORS, MAX_TERMINAL_COLORS);

  for (short i = 0; i < COLORS_AVAILABLE; i++) {
    const short grayscale_value = i * NCURSES_COLOR_COMPONENT_MAX / COLORS_AVAILABLE;
    init_color(i, grayscale_value, grayscale_value, grayscale_value);
  }

  return COLORS_AVAILABLE;
}

curses_color_t ncurses_find_best_initialized_color_number(RGBColor& input) {
  curses_color_t best_color = -1;
  double best_distance = (double)INT32_MAX;
  for (curses_color_t i = 0; i < available_colors; i++) {
    RGBColor current_color = ncurses_get_color_number_content(i);
    double distance = current_color.distance_squared(input);
    if (distance < best_distance) {
      best_color = i;
      best_distance = distance;
    }
  }

  return best_color;
}

curses_color_pair_t ncurses_find_best_initialized_color_pair(RGBColor& input) {
  curses_color_pair_t best_pair = -1;
  double best_distance = (double)INT32_MAX;
  for (curses_color_pair_t i = 0; i < available_color_pairs; i++) {
    ColorPair color_pair = ncurses_get_pair_number_content(i);
    const double distance = color_pair.bg.distance_squared(input);
    if (distance < best_distance) {
      best_pair = i;
      best_distance = distance;
    }
  }

  return best_pair;
}

#undef ASCII_VIDEO_TERMCOLOR_INTERNAL_IMPLEMENTATION