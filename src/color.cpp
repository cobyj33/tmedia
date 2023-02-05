#include <cstdlib>
#include <vector>
#include <stdexcept>

#include "wmath.h"
#include "color.h"

extern "C" {
#include <ncurses.h>
}


const int COLOR_MAP_SIDE = 7;
const int MAX_TERMINAL_COLORS = 256;
const int DEFAULT_TERMINAL_COLOR_COUNT = 8;

int rgb255_to_rgb1000_single(uint8_t val);
RGBColor rgb255_to_rgb1000(RGBColor& color);


RGBColor RGBColor::BLACK = RGBColor(0, 0, 0);
RGBColor RGBColor::WHITE = RGBColor(255, 255, 255);


double RGBColor::distance_squared(RGBColor& other) const {
    // credit to https://www.compuphase.com/cmetric.htm 
    long rmean = ( (long)this->red + (long)other.red ) / 2;
    long r = (long)this->red - (long)other.red;
    long g = (long)this->green - (long)other.green;
    long b = (long)this->blue - (long)other.blue;
    return (((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8);
}

double RGBColor::distance(RGBColor& other) const {
    // credit to https://www.compuphase.com/cmetric.htm 
    return std::sqrt(this->distance_squared(other));
}


RGBColor get_average_color(std::vector<RGBColor>& colors) {
    if (colors.size() == 0) {
        throw std::runtime_error("Cannot find average color value of empty vector");
    }

    int sums[3] = {0, 0, 0};
    for (int i = 0; i < colors.size(); i++) {
        sums[0] += (double)colors[i].red * colors[i].red;
        sums[1] += (double)colors[i].green * colors[i].green;
        sums[2] += (double)colors[i].blue * colors[i].blue;
    }

    return RGBColor(std::sqrt(sums[0]/colors.size()), std::sqrt(sums[1]/colors.size()), std::sqrt(sums[2]/colors.size()));
}

int RGBColor::get_grayscale_value() const {
    return get_grayscale(this->red, this->green, this->blue);
}


int get_grayscale(int r, int g, int b) {
    return (int)(0.299 * r + 0.587 * g + 0.114 * b);
}

RGBColor RGBColor::get_complementary() const {
    return RGBColor(255 - this->red, 255 - this->green, 255 - this->blue );
} 


RGBColor RGBColor::create_grayscale() const {
    int value = get_grayscale(this->red, this->green, this->blue);
    return RGBColor(value);
}

bool RGBColor::equals(RGBColor& other) const {
    return this->red == other.red && this->green == other.green && this->blue == other.blue;
}


void ncurses_init_color_number_rgb(RGBColor& color, int init_index) {
    RGBColor output = rgb255_to_rgb1000(color);
    init_color(init_index, output.red, output.green, output.blue);
}

RGBColor ncurses_get_color_number_content(int color) {
    short r, g, b;
    color_content(color, &r, &g, &b);
    double conversion_factor = 255.0 / 1000;
    return RGBColor((uint8_t)(r * conversion_factor), (uint8_t)(g * conversion_factor), (uint8_t)(b * conversion_factor));
}

std::pair<RGBColor, RGBColor> ncurses_get_pair_number_content(int pair) {
    short fgi, bgi;
    pair_content(pair, &fgi, &bgi);
    return std::make_pair<RGBColor, RGBColor>(ncurses_get_color_number_content(fgi), ncurses_get_color_number_content(bgi));
}

typedef struct pair_distance {
    int pair;
    double distance;
} pair_distance;


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
                RGBColor color = ( r * 255 / (COLOR_MAP_SIDE - 1), g * 255 / (COLOR_MAP_SIDE - 1), b * 255 / (COLOR_MAP_SIDE - 1) );
                color_map[r][g][b] = ncurses_find_best_initialized_color_number(color);
            }
        }
    }

}

void ncurses_init_color_pairs_map() {
    for (int r = 0; r < COLOR_MAP_SIDE; r++) {
        for (int g = 0; g < COLOR_MAP_SIDE; g++) {
            for (int b = 0; b < COLOR_MAP_SIDE; b++) {
                RGBColor color = ( r * 255 / (COLOR_MAP_SIDE - 1), g * 255 / (COLOR_MAP_SIDE - 1), b * 255 / (COLOR_MAP_SIDE - 1) );
                color_pairs_map[r][g][b] = find_best_initialized_color_pair(color);
            }
        }
    }
}

void initialize_colors() {
    if (has_colors()) {
        available_colors = 8;
        if (can_change_color()) {
            ncurses_init_default_color_palette();
        }
        ncurses_init_color_map();
    }
}

void initialize_new_colors(std::vector<RGBColor>& input) {
    if (has_colors()) {
        available_colors = 8;
        if (can_change_color()) {
            ncurses_init_color_palette(input);
        }
        ncurses_init_color_map();
    }
}

void initialize_color_pairs() {
    if (!has_colors()) {
        return;
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

RGBColor find_closest_color(RGBColor& input, std::vector<RGBColor>& colors) {
    int index = find_closest_color_index(input, colors);
    return colors[index];
}

int find_closest_color_index(RGBColor& input, std::vector<RGBColor>& colors) {
    int best_color;
    if (colors.size() == 0) {
        return -1;
        throw std::runtime_error("Cannot find closest color index, colors list is empty ");
    }

    double best_distance = (double)INT32_MAX;
    for (int i = 0; i < colors.size(); i++) {
        double distance = colors[i].distance_squared(input);
        if (distance < best_distance) {
            best_color = i;
            best_distance = distance;
        }
    }

    return best_color;
}

int ncurses_find_best_initialized_color_number(RGBColor& input) {
    if (!has_colors()) {
        return -1;
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

    return color_map[(int)input.red * (COLOR_MAP_SIDE - 1) / 255][(int)input.green * (COLOR_MAP_SIDE - 1) / 255][(int)input.blue * (COLOR_MAP_SIDE - 1) / 255];
}

int get_closest_ncurses_color_pair(RGBColor& input) {
    if (!has_colors()) {
        return -1;
    }

    return color_pairs_map[(int)input.red * (COLOR_MAP_SIDE - 1) / 255][(int)input.green * (COLOR_MAP_SIDE - 1) / 255][(int)input.blue * (COLOR_MAP_SIDE - 1) / 255];
}


int find_best_initialized_color_pair(RGBColor& input) {
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

int rgb255_to_rgb1000_single(uint8_t val) {
    return (int)val * 1000 / 255; 
}

RGBColor rgb255_to_rgb1000(RGBColor& color) {
    return RGBColor(rgb255_to_rgb1000_single(color.red), rgb255_to_rgb1000_single(color.green), rgb255_to_rgb1000_single(color.blue));
}