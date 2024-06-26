#ifndef TMEDIA_TM_CURSES_H
#define TMEDIA_TM_CURSES_H
/**
 * @file tmedia/tmcurses/tmcurses.h
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

#include <tmedia/image/palette.h>
#include <vector>
#include <string>
#include <string_view>

class RGB24;

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

/**
 * This should be called before curses is used at all within tmedia.
 * 
 * No-op if tmcurses is already initialized
*/
void tmcurses_init();

/**
 * Way to check if tmcurses has been initialized with tmcurses_init() and has
 * not been uninitialized with tmcurses_uninit()
*/
bool tmcurses_is_initialized();

/**
 * Returns if tmcurses can use colors in this terminal.
 * 
 * Returns false if curses has_colors() returns false or the environment
 * variable NO_COLOR is defined and non-empty (https://no-color.org/)
*/
bool tmcurses_has_colors();

/**
 * Returns if tmcurses can change colors defined in the terminal.
 * 
 * Returns false if curses has_colors() returns false or the environment
 * variable NO_COLOR is defined and non-empty (https://no-color.org/)
*/
bool tmcurses_can_change_colors();

/**
 * This should be called at the end of the program, even before termination,
 * in order to restore the terminal to its original state
 * 
 * Results in a no-op if tmcurses was not initalized with
 * tmcurses_init previously
*/
void tmcurses_uninit();

enum class TMAlign {
  LEFT = 0,
  RIGHT,
  CENTER
};

struct Style {
  int row;
  int col;
  int width;
  int height;
  TMAlign align;
  int padding_left;
  int padding_right;
  int padding_top;
  int padding_bottom;
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

void tm_mvwaddstr_label(WINDOW* window, TMLabelStyle label_style, std::string_view str);
void tm_mvwprintw_label(WINDOW* window, TMLabelStyle label_style, const char* format, ...);

void wfill_line(WINDOW* window, int y, int x, int width, char ch);
void wfill_box(WINDOW* window, int y, int x, int width, int height, char ch);
void werasebox(WINDOW* window, int y, int x, int width, int height);

/**
 * @brief Initialize ncurses's color palette for a given preset color palette. 
 */
void tmcurses_set_color_palette(TMNCursesColorPalette);
void tmcurses_set_color_palette_custom(const Palette&);

/**
 * @brief Find the closest registered ncurses color pair integer to the inputted RGB24.
 * 
 * Color pairs are used to apply attributes to printed terminal colors
 * @returns The closest registered ncurses color pair attribute index
 */
curses_color_pair_t get_closest_tmcurses_color_pair(const RGB24& input);

/**
 * @brief Find the closest registered ncurses color integer to the inputted RGB24.
 * @returns The closest registered ncurses color pair attribute index
 */
curses_color_t get_closest_tmcurses_color(const RGB24& input);

#endif