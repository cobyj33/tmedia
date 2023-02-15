
#include <cstdarg>

#include "curses_helpers.h"
#include "color.h"

extern "C" {
#include <ncurses.h>
}

void wfill_box(WINDOW* window, int row, int col, int width, int height, char ch) {
    for (int curr_row = row; curr_row <= row + height; row++) {
        for (int curr_col = col; curr_col <= col + width; col++) {
            mvwaddch(window, curr_row, curr_col, ch);
        }
    }
}

void werasebox(WINDOW* window, int row, int col, int width, int height) {
    wfill_box(window, row, col, width, height, ' ');
}

int printwc(int color_pair, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    attron(color_pair);
    int result = vw_printw(stdscr, format, args);
    attroff(color_pair);

    va_end(args);
    return result;
}

int wprintwc(WINDOW* window, int color_pair, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    attron(color_pair);
    int result = vw_printw(window, format, args);
    attroff(color_pair);

    va_end(args);
    return result;
}
