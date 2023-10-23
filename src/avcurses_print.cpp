#include "avcurses.h"

#include "asv_string.h" 
extern "C" {
  #include <curses.h>
}

#include <cstdarg>
#include <string>

void wprintw_center(WINDOW* window, int row, int col, int width, const char* format, ...) {
  int center_col = col + width / 2;
  va_list args;
  va_start(args, format);

  std::string str = vsnprintf_str(format, args);
  str = str_bound(str, width);
  wmove(window, row, center_col - str.length() / 2);
  waddstr(window, str.c_str());
  
  va_end(args);
}

void wprintw_left(WINDOW* window, int y, int x, int width, const char* format, ...) {
  va_list args;
  va_start(args, format);

  std::string str = vsnprintf_str(format, args);
  str = str_bound(str, width);
  wmove(window, y, x);
  waddstr(window, str.c_str());

  va_end(args);
}

void wprintw_right(WINDOW* window, int y, int x, int width, const char* format, ...) {
  va_list args;
  va_start(args, format);

  std::string str = vsnprintf_str(format, args);
  str = str_bound(str, width);

  int move_x = x + width - str.length();

  wmove(window, y, move_x);
  waddstr(window, str.c_str());

  va_end(args);
}

void wfill_box(WINDOW* window, int y, int x, int width, int height, char ch) {
    for (int row = y; row < y + height; row++) {
        for (int col = x; col < x + width; col++) {
            mvwaddch(window, row, col, ch);
        }
    }
}

void werasebox(WINDOW* window, int y, int x, int width, int height) {
    wfill_box(window, y, x, width, height, ' ');
}