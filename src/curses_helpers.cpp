
#include <cstdarg>

#include "curses_helpers.h"
#include "color.h"

extern "C" {
#include <ncurses.h>
}

void wfill_box(WINDOW* window, int y, int x, int width, int height, char ch) {
    for (int row = y; row <= y + height; row++) {
        for (int col = x; col <= x + width; col++) {
            mvwaddch(window, row, col, ch);
        }
    }
}

void werasebox(WINDOW* window, int y, int x, int width, int height) {
    wfill_box(window, y, x, width, height, ' ');
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
