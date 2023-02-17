#ifndef ASCII_VIDEO_TERM_COLOR
#define ASCII_VIDEO_TERM_COLOR

#include <vector>

#include "color.h"

void ncurses_initialize_grayscale_colors();
void ncurses_initialize_colors();

int get_closest_ncurses_color_pair(RGBColor& input);
int get_closest_ncurses_color(RGBColor& input);
int get_closest_ncurses_grayscale_color_pair(RGBColor& input);
int get_closest_ncurses_grayscale_color(RGBColor& input);

#endif