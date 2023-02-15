#ifndef ASCII_VIDEO_CURSES_HELPERS
#define ASCII_VIDEO_CURSES_HELPERS

#include "color.h"
#include <vector>

extern "C" {
    #include <ncurses.h>
}

void wfill_box(WINDOW* window, int row, int col, int width, int height, char ch);
void werasebox(WINDOW* window, int row, int col, int width, int height);

// void printwrgb(char* line, std::vector<RGBColor>& colors);
int printwc(int color_pair, const char* format, ...);
int wprintwc(WINDOW* window, int color_pair, const char* format, ...);

#endif
