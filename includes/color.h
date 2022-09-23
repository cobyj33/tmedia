#pragma once
#include <cstdint>

typedef uint8_t rgb[3];
typedef int rgb_i32[3];

int get_closest_color_pair(rgb input);
int get_closest_color(rgb input);
bool get_most_common_colors(rgb* output, int k, rgb* colors, int nb_colors, int* actual_output_size);

int find_best_initialized_color_pair(rgb input);
int find_best_initialized_color(rgb input);
int find_closest_color_index(rgb input, rgb* colors, int nb_colors);
void find_closest_color(rgb input, rgb* colors, int nb_colors, rgb output);

void initialize_color_pairs();
void initialize_colors();
void initialize_new_colors(rgb* input, int len);
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
void rgb_get_values(rgb rgb, uint8_t* r, uint8_t* g, uint8_t* b);

void get_average_color(rgb output, rgb* colors, int len);
bool quantize_image(rgb* output, int output_len, rgb* colors, int nb_colors);

