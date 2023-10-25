#include "avcurses.h"

#include "asv_string.h" 
extern "C" {
  #include <curses.h>
}

#include <cstdarg>
#include <string>

void mvwaddstr_center(WINDOW* window, int row, int col, int width, const std::string& str) {
  int center_col = col + width / 2;
  std::string bounded_str = str_bound(str, width);
  wmove(window, row, center_col - bounded_str.length() / 2);
  waddstr(window, bounded_str.c_str());
}

void mvwaddstr_left(WINDOW* window, int row, int col, int width, const std::string& str) {
  std::string bounded_str = str_bound(str, width);
  wmove(window, row, col);
  waddstr(window, bounded_str.c_str());
}

void mvwaddstr_right(WINDOW* window, int row, int col, int width, const std::string& str) {
  std::string bounded_str = str_bound(str, width);
  int move_col = col + width - bounded_str.length();
  wmove(window, row, move_col);
  waddstr(window, bounded_str.c_str());
}

void mvwprintw_center(WINDOW* window, int row, int col, int width, const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::string str = vsprintf_str(format, args);
  mvwaddstr_center(window, row, col, width, str);
  va_end(args);
}

void mvwprintw_left(WINDOW* window, int row, int col, int width, const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::string str = vsprintf_str(format, args);
  mvwaddstr_left(window, row, col, width, str);
  va_end(args);
}

void mvwprintw_right(WINDOW* window, int row, int col, int width, const char* format, ...) {
  va_list args;
  va_start(args, format);

  std::string str = vsprintf_str(format, args);
  mvwaddstr_right(window, row, col, width, str);
  
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