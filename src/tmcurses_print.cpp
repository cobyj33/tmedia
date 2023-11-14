#include "tmcurses.h"

#include "tmedia_string.h" 
extern "C" {
  #include <curses.h>
}

#include <cstdarg>
#include <string>

  
TMLabelStyle::TMLabelStyle() {
  this->row = 0;
  this->col = 0;
  this->width = 0;
  this->align = TMAlign::LEFT;
  this->margin_left = 0;
  this->margin_right = 0;
}

TMLabelStyle::TMLabelStyle(int row, int col, int width, TMAlign align, int margin_left, int margin_right) {
  this->row = row;
  this->col = col;
  this->width = width;
  this->align = align;
  this->margin_left = margin_left;
  this->margin_right = margin_right;
}

void tm_mvwaddstr_label(WINDOW* window, TMLabelStyle label_style, const std::string& str) {
  const int requested_text_area_col_start = label_style.col + label_style.margin_left;
  const int requested_text_area_col_end = label_style.col + label_style.width - label_style.margin_right; // not inclusive
  if (requested_text_area_col_end <= requested_text_area_col_start) return; // invalid or negative request

  int text_area_col_start = requested_text_area_col_start;
  int text_area_col_end = requested_text_area_col_end; // not inclusive

  if (text_area_col_end >= COLS) text_area_col_end = COLS - 1; // bound to right edge
  if (text_area_col_start < 0) text_area_col_start = 0; // bound to left edge

  int text_area_width = text_area_col_end - text_area_col_start;
  int text_area_col_center = (text_area_col_start + text_area_col_end) / 2;

  if (text_area_width <= 0 ||
  text_area_col_start < 0 || text_area_col_start >= COLS ||
  text_area_col_end <= 0 || text_area_col_end > COLS) {
    return; // if still invalid, just abort
  }

  std::string bounded_str = str_bound(str, text_area_width);

  switch (label_style.align) {
    case TMAlign::LEFT: mvwaddstr(window, label_style.row, text_area_col_start, bounded_str.c_str()); break;
    case TMAlign::CENTER: mvwaddstr(window, label_style.row, text_area_col_center - (bounded_str.length() / 2), bounded_str.c_str()); break;
    case TMAlign::RIGHT: mvwaddstr(window, label_style.row, text_area_col_end - bounded_str.length(), bounded_str.c_str()); break;
  }
}

void tm_mvwprintw_label(WINDOW* window, TMLabelStyle label_style, const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::string str = vsprintf_str(format, args);
  tm_mvwaddstr_label(window, label_style, str);
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