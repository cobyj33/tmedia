#ifndef TMEDIA_TM_CURSES_H
#define TMEDIA_TM_CURSES_H
/**
 * @file tmcurses.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Custom routines initialize curses, print to screen, and handle terminal colors
 * @version 0.1
 * @date 2023-03-15
 * 
 * tmcurses acts as a thin wrapper over initialization, color indexing, and 
 * uninitializing ncurses for tmedia specifically. 
 * 
 * Note that all color routines simply commit no-ops if colors are not available
 * in the current terminal. Make sure to design so that color is not necessary for
 * understanding program output input, but instead color is an enhancement on
 * what is already being output.
 * 
 * @copyright MIT License (c) 2023 (see LICENSE)
 */

#include "palette.h"
#include <vector>
#include <string>

class RGBColor;

extern "C" {
  #include <curses.h>
}

/**
 * Note that all color routines of tmcurses simply
*/

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

/**
 * Encapsulates the style guides for 
*/
struct TMLabelStyle {
  int row;
  int col;
  int width;
  TMAlign align;
  int margin_left;
  int margin_right;

  TMLabelStyle();
  TMLabelStyle(int row, int col, int width, TMAlign align, int margin_left, int margin_right);
};

void tm_mvwaddstr_label(WINDOW* window, TMLabelStyle label_style, const std::string& str);
void tm_mvwprintw_label(WINDOW* window, TMLabelStyle label_style, const char* format, ...);

void wfill_box(WINDOW* window, int y, int x, int width, int height, char ch);
void werasebox(WINDOW* window, int y, int x, int width, int height);

/**
 * @brief Initialize ncurses's color palette for a given preset color palette. 
 */
void ncurses_set_color_palette(TMNCursesColorPalette);
void ncurses_set_color_palette_custom(const Palette&);

/**
 * @brief Find the closest registered ncurses color pair integer to the inputted RGBColor.
 * 
 * Color pairs are used to apply attributes to printed terminal colors
 * @returns The closest registered ncurses color pair attribute index
 */
curses_color_pair_t get_closest_ncurses_color_pair(const RGBColor& input);

/**
 * @brief Find the closest registered ncurses color integer to the inputted RGBColor.
 * @returns The closest registered ncurses color pair attribute index
 */
curses_color_t get_closest_ncurses_color(const RGBColor& input);

#endif