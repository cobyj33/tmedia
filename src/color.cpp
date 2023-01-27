#include <wmath.h>
#include "color.h"

#include <ncursescpp.h>
#include <cstdlib>

void init_color_rgb(rgb color, int init_index) {
    rgb_i32 output;
    rgb255_to_rgb1000(color, output);
    ncurses::init_color(init_index, output[0], output[1], output[2]);
}

void get_color_content(int color, rgb output) {
    short r, g, b;
    ncurses::color_content(color, &r, &g, &b);
    double conversion_factor = 255.0 / 1000;
    rgb_set(output, (uint8_t)(r * conversion_factor), (uint8_t)(g * conversion_factor), (uint8_t)(b * conversion_factor));
}

void get_pair_content(int pair, rgb fg, rgb bg) {
    short fgi, bgi;
    ncurses::pair_content(pair, &fgi, &bgi);
    get_color_content(fgi, fg);
    get_color_content(bgi, bg);
}

typedef struct pair_distance {
    int pair;
    double distance;
} pair_distance;

#define COLOR_MAP_SIDE 7
#define MAX_TERMINAL_COLORS 256
int color_pairs_map[COLOR_MAP_SIDE][COLOR_MAP_SIDE][COLOR_MAP_SIDE];
int color_map[COLOR_MAP_SIDE][COLOR_MAP_SIDE][COLOR_MAP_SIDE];
int available_colors = 0;
int available_color_pairs = 0;

int get_next_nearest_perfect_square(double num) {
    double nsqrt = std::sqrt(num);
    if (nsqrt == (int)nsqrt) {
        return std::pow(std::ceil(std::sqrt(num) + 1), 2);
    }

    return std::pow(std::ceil(std::sqrt(num)), 2);
}


int quantize_image(rgb* output, int output_len, rgb* colors, int nb_colors) {
    //output_len_cbrt : nb boxes across the color space
    //output_len: nb_boxes to split color space into
    double output_len_cbrt = cbrt(output_len);
    double box_side_size = 255 / output_len_cbrt;
    int nb_buckets = get_next_nearest_perfect_square(output_len + 1);
    rgb* buckets[nb_buckets];
    int bucket_lengths[nb_buckets];
    int bucket_capacities[nb_buckets];

    for (int i = 0; i < nb_buckets; i++) {
        bucket_lengths[i] = 0;
        bucket_capacities[i] = 0;
        buckets[i] = NULL;
    }

    for (int c = 0; c < nb_colors; c++) {
        uint8_t r, g, b;
        rgb_get_values(colors[c], &r, &g, &b);
        double box_coordinates[3] = { ((double)r / box_side_size), ((double)g / box_side_size), ((double)b / box_side_size)  };
        int target_box = box_coordinates[0] * sqrt(nb_buckets) + box_coordinates[1] * cbrt(nb_buckets) + box_coordinates[2]; 

        if (buckets[target_box] == NULL) {
            buckets[target_box] = (rgb*)malloc(sizeof(rgb) * 10);
            if (buckets[target_box] == NULL) {
                continue;
            }
            bucket_capacities[target_box] = 10;
        }

        if (bucket_lengths[target_box] >= bucket_capacities[target_box]) {
            rgb* tmp_bucket = (rgb*)realloc(buckets[target_box], sizeof(rgb) * bucket_capacities[target_box] * 2);
            if (tmp_bucket != NULL) {
                buckets[target_box] = tmp_bucket;
                bucket_capacities[target_box] *= 2; 
            }
        }

        if (bucket_lengths[target_box] < bucket_capacities[target_box]) {
            rgb_copy(buckets[target_box][bucket_lengths[target_box]], colors[c]);
            bucket_lengths[target_box]++;
        }
    }

    for (int i = 0; i < output_len; i++) {
        get_average_color(output[i], buckets[i], bucket_lengths[i]);
    }

    for (int i = 0; i < nb_buckets; i++) {
        if (buckets[i] != NULL) {
            free(buckets[i]);
        }
    }

    return 1;
}

void init_default_color_palette() {
    if (!ncurses::has_colors() || !ncurses::can_change_color()) {
        return;
    }

    available_colors = 8;
    int color_index = 8;
    int colors_to_add = std::min(COLORS - 8, MAX_TERMINAL_COLORS - 8);
    double box_size = 255 / cbrt(colors_to_add);
    for (double r = 0; r < 255; r += box_size) {
        for (double g = 0; g < 255; g += box_size) {
            for (double b = 0; b < 255; b += box_size) {
                ncurses::init_color(color_index, r * 1000 / 255, g * 1000 / 255, b * 1000 / 255);
                color_index++;
            }
        }
    }

    available_colors += colors_to_add;
}

void init_color_palette(rgb* input, int len) {
    if (!ncurses::has_colors() || !ncurses::can_change_color()) {
        return;
    }

    available_colors = 8;
    int colors_to_add = std::min(len, std::min(COLORS - 8, MAX_TERMINAL_COLORS - 8));
    for (int i = 0; i < colors_to_add; i++) {
        ncurses::init_color(i + 8, (short)input[i][0] * 1000 / 255, (short)input[i][1] * 1000 / 255, (short)input[i][2] * 1000 / 255);
    }

    available_colors += colors_to_add;
}

void init_color_map() {
    rgb colors[available_colors];


    for (int r = 0; r < COLOR_MAP_SIDE; r++) {
        for (int g = 0; g < COLOR_MAP_SIDE; g++) {
            for (int b = 0; b < COLOR_MAP_SIDE; b++) {
                rgb color = { (uint8_t)(r * 255 / (COLOR_MAP_SIDE - 1)), (uint8_t)(g * 255 / (COLOR_MAP_SIDE - 1)), (uint8_t)(b * 255 / (COLOR_MAP_SIDE - 1)) };
                color_map[r][g][b] = find_best_initialized_color(color);
            }
        }
    }
}

void init_color_pairs_map() {
    for (int r = 0; r < COLOR_MAP_SIDE; r++) {
        for (int g = 0; g < COLOR_MAP_SIDE; g++) {
            for (int b = 0; b < COLOR_MAP_SIDE; b++) {
                rgb color = { (uint8_t)(r * 255 / (COLOR_MAP_SIDE - 1)), (uint8_t)(g * 255 / (COLOR_MAP_SIDE - 1)), (uint8_t)(b * 255 / (COLOR_MAP_SIDE - 1)) };
                color_pairs_map[r][g][b] = find_best_initialized_color_pair(color);
            }
        }
    }
}

void initialize_colors() {
    if (ncurses::has_colors()) {
        available_colors = 8;
        if (ncurses::can_change_color()) {
            init_default_color_palette();
        }
        init_color_map();
    }
}

void initialize_new_colors(rgb* input, int len) {
    if (ncurses::has_colors()) {
        available_colors = 8;
        if (ncurses::can_change_color()) {
            init_color_palette(input, len);
        }
        init_color_map();
    }
}

void initialize_color_pairs() {
    if (!ncurses::has_colors()) {
        return;
    }

    available_color_pairs = 0;
    for (int i = 0; i < available_colors; i++) {
        ncurses::erase();
        rgb color;
        rgb complementary;
        get_color_content(i, color);
        rgb_complementary(complementary, color);
        ncurses::init_pair(i, get_closest_color(complementary), i);
        available_color_pairs++;
    }

    init_color_pairs_map();
}

void find_closest_color(rgb input, rgb* colors, int nb_colors, rgb output) {
    rgb black = { 0, 0, 0 };
    int index = find_closest_color_index(input, colors, nb_colors);
    if (index == -1) {
        rgb_copy(output, black);
        return;
    }
    rgb_copy(output, colors[index]);
}

int find_closest_color_index(rgb input, rgb* colors, int nb_colors) {
    int best_color = -1;
    if (nb_colors < 0) {
        return -1;
    }

    double best_distance = (double)INT32_MAX;
    for (int i = 0; i < nb_colors; i++) {
        double distance = color_distance_squared(colors[i], input);
        if (distance < best_distance) {
            best_color = i;
            best_distance = distance;
        }
    }

    return best_color;
}

int find_best_initialized_color(rgb input) {
    if (!ncurses::has_colors()) {
        return -1;
    }

    int best_color = -1;
    double best_distance = (double)INT32_MAX;
    for (int i = 0; i < available_colors; i++) {
        rgb current_color;
        get_color_content(i, current_color);
        double distance = color_distance_squared(current_color, input);
        if (distance < best_distance) {
            best_color = i;
            best_distance = distance;
        }
    }

    return best_color;
}

int get_closest_color(rgb input) {
    if (!ncurses::has_colors()) {
        return -1;
    }

    /* return color_map[(int)input[0] * COLOR_MAP_SIDE / 255][(int)input[1] * COLOR_MAP_SIDE / 255][(int)input[2] * COLOR_MAP_SIDE / 255]; */
    return color_map[(int)input[0] * (COLOR_MAP_SIDE - 1) / 255][(int)input[1] * (COLOR_MAP_SIDE - 1) / 255][(int)input[2] * (COLOR_MAP_SIDE - 1) / 255];
}

int get_closest_color_pair(rgb input) {
    if (!ncurses::has_colors()) {
        return -1;
    }

    return color_pairs_map[(int)input[0] * (COLOR_MAP_SIDE - 1) / 255][(int)input[1] * (COLOR_MAP_SIDE - 1) / 255][(int)input[2] * (COLOR_MAP_SIDE - 1) / 255];
}


int find_best_initialized_color_pair(rgb input) {
    if (!ncurses::has_colors()) {
        return -1;
    }

    int best_pair = -1;
    double best_distance = (double)INT32_MAX;
    for (int i = 0; i < available_color_pairs; i++) {
        rgb current_fg, current_bg;
        get_pair_content(i, current_fg, current_bg);
        double distance = color_distance_squared(current_bg, input);
        if (distance < best_distance) {
            best_pair = i;
            best_distance = distance;
        }
    }

    return best_pair;
}



double color_distance_squared(rgb first, rgb second) {
    // credit to https://www.compuphase.com/cmetric.htm 
    long rmean = ( (long)first[0] + (long)second[0]  ) / 2;
    long r = (long)first[0] - (long)second[0];
    long g = (long)first[1] - (long)second[1];
    long b = (long)first[2] - (long)second[2];
    return (((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8);
}

double color_distance(rgb first, rgb second) {
    // credit to https://www.compuphase.com/cmetric.htm 
    return std::sqrt(color_distance_squared(first, second));
}

void get_average_color(rgb output, rgb* colors, int len) {
    if (len <= 0) {
        rgb_set(output, 0, 0, 0);
        return;
    }

    double sums[3] = { 0, 0, 0 };
    for (int i = 0; i < len; i++) {
        sums[0] += (double)colors[i][0] * colors[i][0];
        sums[1] += (double)colors[i][1] * colors[i][1];
        sums[2] += (double)colors[i][2] * colors[i][2];
    }

    rgb_set(output, (uint8_t)sqrt(sums[0]/len), (uint8_t)sqrt(sums[1]/len), (uint8_t)sqrt(sums[2]/len));
}

void rgb_copy(rgb dest, rgb src) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}


void rgb_set(rgb rgb, uint8_t r, uint8_t g, uint8_t b) {
    rgb[0] = r;
    rgb[1] = g;
    rgb[2] = b;
}

int rgb_equals(rgb first, rgb second) {
    return first[0] == second[0] && first[1] == second[1] && first[2] == second[2] ? 1 : 0;
}


int get_grayscale(int r, int g, int b) {
    return (int)(0.299 * r + 0.587 * g + 0.114 * b);
}

int get_grayscale_rgb(rgb rgb_color) {
    return get_grayscale(rgb_color[0], rgb_color[1], rgb_color[2]);
}

int rgb255_to_rgb1000_single(uint8_t val) {
    return (int)val * 1000 / 255;
}

void rgb255_to_rgb1000(rgb colors, int output[3]) {
    for (int i = 0; i < 3; i++) {
        output[i] = rgb255_to_rgb1000_single(colors[i]);
    }
}

void rgb_complementary(rgb dest, rgb src) {
    rgb_set(dest, 255 - src[0], 255 - src[1], 255 - src[2] );
}


void rgb_get_values(rgb rgb, uint8_t* r, uint8_t* g, uint8_t* b) {
    *r = rgb[0];
    *g = rgb[1];
    *b = rgb[2];
}
