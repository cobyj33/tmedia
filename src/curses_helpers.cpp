
#include <cstdarg>
#include <curses_helpers.h>

extern "C" {
#include <ncurses.h>
}

const int CUSTOM_COLOR = 16;
const int CUSTOM_COLOR_PAIR = 1;

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

void printwrgb(char* line, rgb* colors, int len) {
    static int color_index = CUSTOM_COLOR;
    static int color_pair = CUSTOM_COLOR_PAIR;

    rgb_i32 output;
    for (int i = 0; i < len; i++) {
        color_index = color_index >= COLORS ? CUSTOM_COLOR : color_index + 1; 
        color_pair = color_pair >= COLOR_PAIRS ? CUSTOM_COLOR_PAIR : color_pair + 1;
       rgb255_to_rgb1000(colors[i], output); 
       init_color(color_index, output[0], output[1], output[2]);
       init_pair(color_pair, color_index, COLOR_BLACK);

       attron(COLOR_PAIR(color_pair)); 
       addch(line[i]);
       attroff(COLOR_PAIR(color_pair)); 
    }
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
