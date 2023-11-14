#ifndef TMEDIA_TM_CURSES_H
#define TMEDIA_TM_CURSES_H
/**
 * @file tmcyrses.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief NCurses routines initialize curses, to pre-load color maps, and to quickly fetch colors based on RGB Values
 * @version 0.1
 * @date 2023-03-15
 * 
 * @copyright MIT License (c) 2023 (see LICENSE)
 */

#include <vector>
#include <string>

#include "color.h"
extern "C" {
  #include <curses.h>
}

typedef short curses_color_pair_t;
typedef short curses_color_t;

enum class TMNCursesColorPalette {  
  RGB,
  GRAYSCALE
};

void ncurses_init();
bool ncurses_is_initialized();
void ncurses_uninit();

enum class TMAlign {
  LEFT = 0,
  RIGHT,
  CENTER
};

struct TMLabelStyle {
  int row;
  int col;
  int width;
  TMAlign align;
  int margin_left;
  int margin_right;

  TMLabelStyle();
};

void tm_mvwaddstr_label(WINDOW* window, TMLabelStyle label_style, const std::string& str);
void tm_mvwprintw_label(WINDOW* window, TMLabelStyle label_style, const char* format, ...);

void wfill_box(WINDOW* window, int y, int x, int width, int height, char ch);
void werasebox(WINDOW* window, int y, int x, int width, int height);

/**
 * @brief Initialize ncurses's color palette for a given enum
 * In it's current implementation, this function will load 7^3 (216) colors into the ncurses color palette
 */
void ncurses_set_color_palette(TMNCursesColorPalette);

/**
 * @brief Find the closest registered ncurses color pair integer to the inputted RGBColor.
 * 
 * Color pairs are used to apply attributes to printed terminal colors
 * @note This function should only be used after either ncurses_initialize_color_palette() or ncurses_initialize_grayscale_color_palette() is run
 * @returns The closest registered ncurses color pair attribute index
 */
curses_color_pair_t get_closest_ncurses_color_pair(const RGBColor& input);

/**
 * @brief Find the closest registered ncurses color integer to the inputted RGBColor.
 * 
 * Colors are used to create ncurses color pairs, which are then used to apply attributes to printed terminal colors
 * Generally, if printing is required, get_closest_ncurses_color_pair is a recommended alternative  
 * @note This function should only be used after either ncurses_initialize_color_palette() or ncurses_initialize_grayscale_color_palette() is run
 * @returns The closest registered ncurses color pair attribute index
 */
curses_color_t get_closest_ncurses_color(const RGBColor& input);

#endif