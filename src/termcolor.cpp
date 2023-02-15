
#include <stdexcept>
#include <cmath>

#include "color.h"
#include "termcolor.h"
#include "wmath.h"


extern "C" {
#include <ncurses.h>
}

const int COLOR_MAP_SIDE = 7;
const int MAX_TERMINAL_COLORS = 256;
const int DEFAULT_TERMINAL_COLOR_COUNT = 8;

/**
 * @brief A mapping of NCURSES color pairs to a 3D Color space, where each side of the color space is COLOR_MAP_SIDE steps long and maps to [red / 255][green / 255][blue / 255]
 * Additionally, the backgrounds of all colors should be a certain color, while the foreground is that color's complementary value
 */
int color_pairs_map[COLOR_MAP_SIDE][COLOR_MAP_SIDE][COLOR_MAP_SIDE];

/**
 * @brief A mapping of NCURSES colors to a 3D Color space, where each side of the color space is COLOR_MAP_SIDE steps long and maps to [red / 255][green / 255][blue / 255]
 */
int color_map[COLOR_MAP_SIDE][COLOR_MAP_SIDE][COLOR_MAP_SIDE];

/**
 * @brief The current amount of available unique colors initialized in NCurses
 */
int available_colors = 0;

/**
 * @brief The current amount of available unique color pairs initialized in NCurses
 */
int available_color_pairs = 0;

int rgb255_to_rgb1000_single(int val) {
    return val * 1000 / 255; 
}

int rgb1000_to_rgb255_single(int val) {
    return val * 255 / 1000; 
}

RGBColor rgb255_to_rgb1000(RGBColor& color) {
    return RGBColor(rgb255_to_rgb1000_single(color.red), rgb255_to_rgb1000_single(color.green), rgb255_to_rgb1000_single(color.blue));
}

RGBColor rgb1000_to_rgb255(RGBColor& color) {
    return RGBColor(rgb1000_to_rgb255_single(color.red), rgb1000_to_rgb255_single(color.green), rgb1000_to_rgb255_single(color.blue));
}

void ncurses_init_color_number_rgb(RGBColor& color, int init_index) {
    RGBColor output = rgb255_to_rgb1000(color);
    init_color(init_index, (short)output.red, (short)output.green, (short)output.blue);
}

RGBColor ncurses_get_color_number_content(int color) {
    short r, g, b;
    color_content(color, &r, &g, &b);
    double conversion_factor = 255.0 / 1000;
    RGBColor color_1000 = RGBColor(r, g, b); // ncurses colors are in the range 0-1000 instead of the standard 0-255
    return rgb1000_to_rgb255(color_1000);
}

std::pair<RGBColor, RGBColor> ncurses_get_pair_number_content(int pair) {
    short fgi, bgi;
    pair_content(pair, &fgi, &bgi);
    return std::make_pair<RGBColor, RGBColor>(ncurses_get_color_number_content(fgi), ncurses_get_color_number_content(bgi));
}

void ncurses_init_default_color_palette() {
    if (!has_colors() || !can_change_color()) {
        return;
    }

    available_colors = DEFAULT_TERMINAL_COLOR_COUNT;
    int color_index = DEFAULT_TERMINAL_COLOR_COUNT;
    int colors_to_add = std::min(COLORS - DEFAULT_TERMINAL_COLOR_COUNT, MAX_TERMINAL_COLORS - DEFAULT_TERMINAL_COLOR_COUNT);
    double box_size = 255 / std::cbrt(colors_to_add);
    for (double r = 0; r < 255; r += box_size) {
        for (double g = 0; g < 255; g += box_size) {
            for (double b = 0; b < 255; b += box_size) {
                init_color(color_index, r * 1000 / 255, g * 1000 / 255, b * 1000 / 255);
                color_index++;
            }
        }
    }

    available_colors += colors_to_add;
}

void ncurses_init_color_palette(std::vector<RGBColor>& input) {
    if (!has_colors() || !can_change_color()) {
        return;
    }

    available_colors = DEFAULT_TERMINAL_COLOR_COUNT;
    int colors_to_add = std::min((int)input.size(), std::min(COLORS - DEFAULT_TERMINAL_COLOR_COUNT, MAX_TERMINAL_COLORS - DEFAULT_TERMINAL_COLOR_COUNT));
    for (int i = 0; i < colors_to_add; i++) {
        init_color(i + DEFAULT_TERMINAL_COLOR_COUNT, (short)input[i].red * 1000 / 255, (short)input[i].green * 1000 / 255, (short)input[i].blue * 1000 / 255);
    }

    available_colors += colors_to_add;
}

void ncurses_init_color_map() {

    for (int r = 0; r < COLOR_MAP_SIDE; r++) {
        for (int g = 0; g < COLOR_MAP_SIDE; g++) {
            for (int b = 0; b < COLOR_MAP_SIDE; b++) {
                RGBColor color( r * 255 / (COLOR_MAP_SIDE - 1), g * 255 / (COLOR_MAP_SIDE - 1), b * 255 / (COLOR_MAP_SIDE - 1) );
                color_map[r][g][b] = ncurses_find_best_initialized_color_number(color);
            }
        }
    }

}

void ncurses_init_color_pairs_map() {
    for (int r = 0; r < COLOR_MAP_SIDE; r++) {
        for (int g = 0; g < COLOR_MAP_SIDE; g++) {
            for (int b = 0; b < COLOR_MAP_SIDE; b++) {
                RGBColor color( r * 255 / (COLOR_MAP_SIDE - 1), g * 255 / (COLOR_MAP_SIDE - 1), b * 255 / (COLOR_MAP_SIDE - 1) );
                color_pairs_map[r][g][b] = ncurses_find_best_initialized_color_pair(color);
            }
        }
    }
}


void ncurses_initialize_colors() {
    if (has_colors()) {
        available_colors = DEFAULT_TERMINAL_COLOR_COUNT;
        if (can_change_color()) {
            ncurses_init_default_color_palette();
        }
        ncurses_init_color_map();
    }
}

void ncurses_initialize_new_colors(std::vector<RGBColor>& input) {
    if (!has_colors()) {
        throw std::runtime_error("Cannot initialize new colors in terminal, terminal does not support colors");
    }

    available_colors = 8;
    if (can_change_color()) {
        ncurses_init_color_palette(input);
    }
    ncurses_init_color_map();
}

void ncurses_initialize_color_pairs() {
    if (!has_colors()) {
        throw std::runtime_error("Cannot initialize color pairs in terminal, terminal does not support colors");
    }

    available_color_pairs = 0;
    for (int i = 0; i < available_colors; i++) {
        erase();
        RGBColor color = ncurses_get_color_number_content(i);
        RGBColor complementary = color.get_complementary();
        init_pair(i, get_closest_ncurses_color(complementary), i);
        available_color_pairs++;
    }

    ncurses_init_color_pairs_map();
}


int ncurses_find_best_initialized_color_number(RGBColor& input) {
    if (!has_colors()) {
        throw std::runtime_error("Cannot find best color number in terminal, terminal does not support colors");
    }

    int best_color = -1;
    double best_distance = (double)INT32_MAX;
    for (int i = 0; i < available_colors; i++) {
        RGBColor current_color = ncurses_get_color_number_content(i);
        double distance = current_color.distance_squared(input);
        if (distance < best_distance) {
            best_color = i;
            best_distance = distance;
        }
    }

    return best_color;
}

int get_closest_ncurses_color(RGBColor& input) {
    if (!has_colors()) {
        throw std::runtime_error("Cannot find closest ncurses color, terminal does not support colors");
    }

    return color_map[input.red * (COLOR_MAP_SIDE - 1) / 255][input.green * (COLOR_MAP_SIDE - 1) / 255][input.blue * (COLOR_MAP_SIDE - 1) / 255];
}

int get_closest_ncurses_color_pair(RGBColor& input) {
    if (!has_colors()) {
        throw std::runtime_error("Cannot find closest ncurses color pair, terminal does not support colors");
    }

    return color_pairs_map[input.red * (COLOR_MAP_SIDE - 1) / 255][input.green * (COLOR_MAP_SIDE - 1) / 255][input.blue * (COLOR_MAP_SIDE - 1) / 255];
}


int ncurses_find_best_initialized_color_pair(RGBColor& input) {
    if (!has_colors()) {
        return -1;
    }

    int best_pair = -1;
    double best_distance = (double)INT32_MAX;
    for (int i = 0; i < available_color_pairs; i++) {
        std::pair<RGBColor, RGBColor> color_pair_data = ncurses_get_pair_number_content(i);
        RGBColor current_fg = color_pair_data.first;
        RGBColor current_bg = color_pair_data.second;
        double distance = current_bg.distance_squared(input), input;
        if (distance < best_distance) {
            best_pair = i;
            best_distance = distance;
        }
    }

    return best_pair;
}
