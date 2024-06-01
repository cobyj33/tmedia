#include <tmedia/tmcurses/tmcurses.h>

#include <tmedia/image/color.h>
#include <tmedia/util/wmath.h>
#include <tmedia/image/palette.h>
#define TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION
#include <tmedia/tmcurses/internal/tmcurses_internal.h>
#undef TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <cfloat>
#include <cstdint>

#include <fmt/format.h>
#include <tmedia/util/defines.h>

extern "C" {
#include <curses.h>
}

static constexpr int COLOR_CUBE_SIDE_LENGTH = 255;
static constexpr int COLOR_MAP_SIDE_NB_CELLS = 7;
static constexpr int COLOR_MAP_CELL_WIDTH = COLOR_CUBE_SIDE_LENGTH / COLOR_MAP_SIDE_NB_CELLS;
static constexpr int MAX_TERMINAL_COLORS = 256;
static constexpr int MAX_TERMINAL_COLOR_PAIRS = 256;

#if 0
// We offset by COLOR_PALETTE_START into the terminal's color indexes. This
// is still technically true though, the first 8 are reserved. It is unused
// for now though.
static constexpr int DEFAULT_TERMINAL_COLOR_COUNT = 8;
#endif

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
 * COLOR_RED or COLOR_GREEN color pair returning from get_tmcurses_color_pair 
*/
bool curses_colors_initialized = false;

/**
 * @brief A RGB mapping of NCURSES color pairs to a 3D Discrete Color space, where each side of the color space is COLOR_MAP_SIDE_NB_CELLS steps long
 * To index into the map, take each channel's ratio against 255 and multiply it against COLOR_MAP_SIDE_NB_CELLS (ex: (int)(red / 255.0 * COLOR_MAP_SIZE))
 * Additionally, the backgrounds of all colors should be a certain color, while the foreground is that color's complementary value
 */
curses_color_pair_t color_pairs_map[COLOR_MAP_SIDE_NB_CELLS][COLOR_MAP_SIDE_NB_CELLS][COLOR_MAP_SIDE_NB_CELLS];
curses_color_t color_map[COLOR_MAP_SIDE_NB_CELLS][COLOR_MAP_SIDE_NB_CELLS][COLOR_MAP_SIDE_NB_CELLS];

int available_color_palette_colors = 0;
int available_color_palette_color_pairs = 0;

// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// ----------------------- PUBLIC API ---------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------

bool tmcurses_has_colors() {
  return has_colors() && !no_color_is_set();
}

bool tmcurses_can_change_colors() {
  return can_change_color() && !no_color_is_set();
}

void tmcurses_init_color() {
  if (!tmcurses_has_colors() || !tmcurses_can_change_colors()) return;
  if (curses_colors_initialized) return;

  start_color();
  if (COLORS <= COLOR_PALETTE_START) return;

  use_default_colors();
  
  available_color_palette_colors = 0;
  available_color_palette_color_pairs = 0;

  tmcurses_init_color_pairs();
  tmcurses_init_color_maps();
  curses_colors_initialized = true;
}

void tmcurses_uninit_color() {
  available_color_palette_colors = 0;
  available_color_palette_color_pairs = 0;
  curses_colors_initialized = false;
}

void tmcurses_set_color_palette(TMNCursesColorPalette colorPalette) {
  if (!curses_colors_initialized) return;

  available_color_palette_colors = 0;

  switch (colorPalette) {
    case TMNCursesColorPalette::RGB:
      available_color_palette_colors = tmcurses_init_rgb_color_palette();
      break;
    case TMNCursesColorPalette::GRAYSCALE:
      available_color_palette_colors = tmcurses_init_grayscale_color_palette();
      break;
  }

  tmcurses_init_color_pairs();
  tmcurses_init_color_maps();
}

void tmcurses_set_color_palette_custom(const Palette& colorPalette) {
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

  tmcurses_init_color_pairs();
  tmcurses_init_color_maps();
}



curses_color_t get_closest_tmcurses_color(RGB24 input) {
  if (!curses_colors_initialized) return 0; // just return a default 0 to no-op

  /*
    Divide by 256 is intentional here, although uint8_t's max is 255.

    Beforehand, the mapping from an RGB unsigned 8-bit value to the color map
    was done with (color channel) * (COLOR_MAP_SIDE_NB_CELLS - 1) / 255, which
    intutively seems correct, since the maximum index of a side of the color
    map is (COLOR_MAP_SIDE_NB_CELLS - 1) and uint8_t's max is 255, so when
    a color channel equals 255, the expression evaluates to
    (COLOR_MAP_SIDE_NB_CELLS) - 1 and is in bounds. However, This means that
    intense red, green, and blue values were rarely accessed by
    calls to get_closest_tmcurses_color except in cases of extreme saturation
    where the r, g, or b channel was equal to 255. For example, if r equals 254,
    254 * (COLOR_MAP_SIDE_NB_CELLS - 1) / 255 would truncate through integer
    division, causing a linear distribution of the values of the channel
    **except** for the final array index.

    Instead, with the function (color channel) * COLOR_MAP_SIDE_NB_CELLS / 256
    and 8-bit channel values, we can guarantee that the expression will never
    actually equal COLOR_MAP_SIDE_NB_CELLS, since (color channel) < 256. Instead,
    (in actual math terms) it would evaluate close to COLOR_MAP_SIDE_NB_CELLS in
    saturated examples, but
    would then be truncated down to COLOR_MAP_SIDE_NB_CELLS - 1 due to integer
    divison, causing channels on the edge of the color map to be caught.

    (I decided to write this comment since this seemed extremely non-intuitive)
  */

  return color_map[static_cast<int>(input.r) * COLOR_MAP_SIDE_NB_CELLS / 256]
                  [static_cast<int>(input.g) * COLOR_MAP_SIDE_NB_CELLS / 256]
                  [static_cast<int>(input.b) * COLOR_MAP_SIDE_NB_CELLS / 256];
}

curses_color_pair_t get_closest_tmcurses_color_pair(RGB24 input) {
  if (!curses_colors_initialized) return 0; // just return a default 0 to no-op

  return color_pairs_map[static_cast<int>(input.r) * COLOR_MAP_SIDE_NB_CELLS / 256]
                        [static_cast<int>(input.g) * COLOR_MAP_SIDE_NB_CELLS / 256]
                        [static_cast<int>(input.b) * COLOR_MAP_SIDE_NB_CELLS / 256];
}

// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// ------------------------- BACKEND ----------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------

// https://no-color.org/
bool no_color_is_set() {
  const char* no_color = getenv("NO_COLOR");
  return no_color != nullptr && no_color[0] != '\0';
}

constexpr const char* tmcurses_color_palette_cstr(TMNCursesColorPalette colorPalette) {
  switch (colorPalette) {
    case TMNCursesColorPalette::RGB: return "rgb";
    case TMNCursesColorPalette::GRAYSCALE: return "grayscale";
  }
  return "unknown";
}

RGB24 tmcurses_get_color_number_content(curses_color_t color) {
  short r, g, b;
  color_content(color, &r, &g, &b);
  return RGB24(static_cast<std::uint8_t>(r * 255 / 1000),
    static_cast<std::uint8_t>(g * 255 / 1000),
    static_cast<std::uint8_t>(b * 255 / 1000));
}

ColorPair tmcurses_get_pair_number_content(curses_color_pair_t pair) {
  short fgi, bgi;
  pair_content(pair, &fgi, &bgi);
  return { tmcurses_get_color_number_content(fgi), tmcurses_get_color_number_content(bgi) };
}

int tmcurses_init_rgb_color_palette() {
  static constexpr double NCURSES_COLOR_COMPONENT_MAX = 1000.;
  const short MAX_COLORS = std::min(COLORS, MAX_TERMINAL_COLORS);
  const short CHANGEABLE_COLORS = MAX_COLORS - COLOR_PALETTE_START;
  if (CHANGEABLE_COLORS <= 0) return 0;

  // simulates walking around a color cube with side length
  // NCURSES_COLOR_COMPONENT_MAX and initializes colors from points along
  // the walk

  double STEPS_PER_COLOR_CUBE_SIDE = std::max(std::floor(std::cbrt(CHANGEABLE_COLORS)), 1.0);
  STEPS_PER_COLOR_CUBE_SIDE -= STEPS_PER_COLOR_CUBE_SIDE > 1.0 ? 1.0 : 0.0;
  const double BOX_SIZE = NCURSES_COLOR_COMPONENT_MAX / STEPS_PER_COLOR_CUBE_SIDE;

  int color_index = COLOR_PALETTE_START;
  for (double r = 0; r <= NCURSES_COLOR_COMPONENT_MAX && color_index < MAX_COLORS; r += BOX_SIZE) {
    for (double g = 0; g <= NCURSES_COLOR_COMPONENT_MAX && color_index < MAX_COLORS; g += BOX_SIZE) {
      for (double b = 0; b <= NCURSES_COLOR_COMPONENT_MAX && color_index < MAX_COLORS; b += BOX_SIZE) {
        init_color(color_index++, static_cast<short>(r), static_cast<short>(g), static_cast<short>(b));
      }
    }
  }

  return color_index - COLOR_PALETTE_START;
}


int tmcurses_init_grayscale_color_palette() {
  static constexpr short NCURSES_COLOR_COMPONENT_MAX = 1000;
  const short MAX_COLORS = std::min(COLORS, MAX_TERMINAL_COLORS);
  const short CHANGEABLE_COLORS = MAX_COLORS - COLOR_PALETTE_START;
  if (CHANGEABLE_COLORS <= 0) return 0;

  for (short i = 0; i < CHANGEABLE_COLORS; i++) {
    // dividing by (CHANGEABLE_COLORS - 1), since the maximum for "i" is
    // (CHANGEABLE_COLORS - 1), therefore we will get the full gray scale
    // from black to white when (i / (CHANGEABLE_COLORS - 1) == 1)
    const short gray = i * NCURSES_COLOR_COMPONENT_MAX / (CHANGEABLE_COLORS - 1);
    init_color(i + COLOR_PALETTE_START, gray, gray, gray);
  }

  return CHANGEABLE_COLORS;
}

void tmcurses_init_color_pairs() {
  const int COLOR_PAIRS_TO_INIT = std::min(MAX_TERMINAL_COLOR_PAIRS, std::min(COLOR_PAIRS, available_color_palette_colors));
  for (int i = 0; i < COLOR_PAIRS_TO_INIT; i++) {
    const curses_color_pair_t color_pair_index = i;
    const curses_color_t color_index = i + COLOR_PALETTE_START;
    const RGB24 color = tmcurses_get_color_number_content(color_index);
    const RGB24 complementary = color.get_comp();
    init_pair(color_pair_index, tmcurses_find_best_initialized_color_number(complementary), color_index);
  }

  available_color_palette_color_pairs = COLOR_PAIRS_TO_INIT;
}

void tmcurses_init_color_maps() {
  for (int r = 0; r < COLOR_MAP_SIDE_NB_CELLS; r++) {
    for (int g = 0; g < COLOR_MAP_SIDE_NB_CELLS; g++) {
      for (int b = 0; b < COLOR_MAP_SIDE_NB_CELLS; b++) {
        const RGB24 color(static_cast<std::uint8_t>(r * 255 / (COLOR_MAP_SIDE_NB_CELLS - 1)),
           static_cast<std::uint8_t>(g * 255 / (COLOR_MAP_SIDE_NB_CELLS - 1)),
           static_cast<std::uint8_t>(b * 255 / (COLOR_MAP_SIDE_NB_CELLS - 1)));

        color_map[r][g][b] = tmcurses_find_best_initialized_color_number(color);
        color_pairs_map[r][g][b] = tmcurses_find_best_initialized_color_pair(color);
      }
    }
  }
}

curses_color_t tmcurses_find_best_initialized_color_number(RGB24 input) {
  curses_color_t best_color_index = -1;
  double best_distance = DBL_MAX;
  for (int i = 0; i < available_color_palette_colors; i++) {
    const curses_color_t color_index = i + COLOR_PALETTE_START;
    const RGB24 current_color = tmcurses_get_color_number_content(color_index);
    const double distance = current_color.dis_sq(input);
    if (distance < best_distance) {
      best_color_index = color_index;
      best_distance = distance;
    }
  }

  return best_color_index;
}

curses_color_pair_t tmcurses_find_best_initialized_color_pair(RGB24 input) {
  curses_color_pair_t best_pair_index = -1;
  double best_distance = DBL_MAX;
  for (int i = 0; i < available_color_palette_color_pairs; i++) {
    const curses_color_pair_t color_pair_index = i;
    const ColorPair color_pair = tmcurses_get_pair_number_content(color_pair_index);
    const double distance = color_pair.bg.dis_sq(input);
    if (distance < best_distance) {
      best_pair_index = color_pair_index;
      best_distance = distance;
    }
  }

  return best_pair_index;
}
