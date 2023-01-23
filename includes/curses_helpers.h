#ifndef ASCII_VIDEO_CURSES_HELPERS
#define ASCII_VIDEO_CURSES_HELPERS

#include "image.h"

extern "C" {
    #include <ncurses.h>
}

void wfill_box(WINDOW* window, int x, int y, int width, int height, char ch);
void werasebox(WINDOW* window, int x, int y, int width, int height);

void printwrgb(char* line, rgb* colors, int len);
int printwc(int color_pair, const char* format, ...);
int wprintwc(WINDOW* window, int color_pair, const char* format, ...);
#endif
