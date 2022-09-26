#ifndef ASCII_VIDEO_CURSES_HELPERS
#define ASCII_VIDEO_CURSES_HELPERS

#include <ncurses.h>
#include "image.h"

void printwrgb(char* line, rgb* colors, int len);
int printwc(int color_pair, const char* format, ...);
int wprintwc(WINDOW* window, int color_pair, const char* format, ...);
#endif
