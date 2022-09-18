#pragma once
#include <cstdint>

typedef uint8_t rgb[3];
typedef int rgb_i32[3];

int get_closest_color_pair(rgb input);
int find_closest_color_pair(rgb input);
int get_closest_color(rgb input);
int find_closest_color(rgb input);

void initialize_color_pairs();
void initialize_colors();
void get_pair_content(int pair, rgb fg, rgb bg);
void get_color_content(int color, rgb output);

void init_color_rgb(rgb color, int init_index);
int rgb255_to_rgb1000_single(uint8_t val);
void rgb255_to_rgb1000(rgb colors, int output[3]);
double color_distance(rgb first, rgb second);
double color_distance_squared(rgb first, rgb second);
int get_grayscale(int r, int g, int b);
int get_grayscale_rgb(rgb rgb_color);
void rgb_set(rgb rgb, uint8_t r, uint8_t g, uint8_t b);
void rgb_copy(rgb dest, rgb src);
void rgb_complementary(rgb dest, rgb src);
bool rgb_equals(rgb first, rgb second);
