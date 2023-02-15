#ifndef ASCII_VIDEO_TERM_COLOR
#define ASCII_VIDEO_TERM_COLOR
#include <vector>

#include "color.h"

int get_closest_ncurses_color_pair(RGBColor& input);
int get_closest_ncurses_color(RGBColor& input);

int ncurses_find_best_initialized_color_pair(RGBColor& input);
int ncurses_find_best_initialized_color_number(RGBColor& input);



void ncurses_initialize_color_pairs();
void ncurses_initialize_colors();
void ncurses_initialize_new_colors(std::vector<RGBColor>& input);
std::pair<RGBColor, RGBColor> ncurses_get_pair_number_content(int pair);
RGBColor ncurses_get_color_number_content(int color);

void ncurses_init_color_number_rgb(RGBColor& color, int init_index);

#endif