
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <map>

#include "audio.h"
#include "color.h"
#include "termcolor.h"
#include "pixeldata.h"
#include "playheadlist.hpp"
#include "image.h"
#include "ascii.h"
#include "icons.h"
#include "media.h"
#include "threads.h"
#include "wmath.h"
#include "wtime.h"
#include "curses_helpers.h"

#include "gui.h"


extern "C" {
#include <ncurses.h>
#include <libavutil/avutil.h>
}

const int KEY_ESCAPE = 27;

void render_movie_screen(PixelData& pixelData, VideoOutputMode gui_state);
void print_pixel_data(PixelData& pixelData, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode);
void print_pixel_data_text(PixelData& pixelData, int bounds_row, int bounds_col, int bounds_width, int bounds_height);

RGBColor get_index_display_color(int index, int length) {
    const double step = (255.0 / 2.0) / length;
    const double color_space_size = 255 - (step * (double)(index / 6));
    int red = index % 6 >= 3 ? color_space_size : 0;
    int green = index % 2 == 0 ? color_space_size : 0;
    int blue = index % 6 < 3 ? color_space_size : 0;
    return RGBColor(red, green, blue);
}


void render_loop(MediaPlayer* player, std::mutex& alter_mutex, GUIState gui_state) {
    WINDOW* inputWindow = newwin(0, 0, 1, 1);
    nodelay(inputWindow, true);
    keypad(inputWindow, true);


    std::unique_lock<std::mutex> lock(alter_mutex, std::defer_lock);
    erase();
    while (player->inUse) {
        
        int input = wgetch(inputWindow);
        if (input == KEY_RESIZE) {
            erase();
            endwin();
            refresh();
        }

        lock.lock();
        PixelData& image = player->displayCache.image;
        lock.unlock();

        render_movie_screen(image, gui_state.get_video_output_mode());
        sleep_for_ms(41);
    }

    delwin(inputWindow);
}


void render_movie_screen(PixelData& pixelData, VideoOutputMode output_mode) {
    print_pixel_data(pixelData, 0, 0, COLS, LINES, output_mode);
    refresh();
}

void print_pixel_data_text(PixelData& pixelData, int bounds_row, int bounds_col, int bounds_width, int bounds_height) {
    PixelData bounded = pixelData.bound(bounds_width, bounds_height);
    const AsciiImage image(bounded, AsciiImage::ASCII_STANDARD_CHAR_MAP);
    int image_start_row = bounds_row + std::abs(image.get_height() - bounds_height) / 2;
    int image_start_col = bounds_col + std::abs(image.get_width() - bounds_width) / 2; 

    for (int row = 0; row < image.get_height(); row++) {
        for (int col = 0; col < image.get_width(); col++) {
            mvaddch(image_start_row + row, image_start_col + col, image.at(row, col).ch);
        } 
    }
}

typedef struct ScreenChar {
    ColorChar character;
    int row;
    int col;
} ScreenChar;


void print_pixel_data(PixelData& pixelData, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode) {
    if (output_mode == VideoOutputMode::TEXT_ONLY) {
        print_pixel_data_text(pixelData, bounds_row, bounds_col, bounds_width, bounds_height);
        return;
    }
    
    if (!has_colors()) {
        throw std::runtime_error("Attempted to print colored text in terminal that does not support color");
    }

    PixelData bounded = pixelData.bound(bounds_width, bounds_height);
    const AsciiImage image(bounded, AsciiImage::ASCII_STANDARD_CHAR_MAP);
    int image_start_row = bounds_row + std::abs(image.get_height() - bounds_height) / 2;
    int image_start_col = bounds_col + std::abs(image.get_width() - bounds_width) / 2; 

    bool grayscale = output_mode == VideoOutputMode::GRAYSCALE || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;
    bool background_only = output_mode == VideoOutputMode::COLORED_BACKGROUND_ONLY || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;

    std::map<int, std::vector<ScreenChar>> colors;
    std::vector<int> color_pairs;
    for (int row = 0; row < image.get_height(); row++) {
        for (int col = 0; col < image.get_width(); col++) {
            ColorChar color_char = image.at(row, col);
            color_char = ColorChar(background_only ? ' ' : color_char.ch, color_char.color);
            int color_pair = grayscale ? get_closest_ncurses_grayscale_color_pair(color_char.color) : get_closest_ncurses_color_pair(color_char.color);
            if (colors.count(color_pair) == 0) {
                colors.emplace(color_pair, std::vector<ScreenChar>());
                color_pairs.push_back(color_pair);
            }
            colors.at(color_pair).push_back({color_char, image_start_row + row, image_start_col + col });
        }
    }

    for (int p = 0; p < color_pairs.size(); p++) {
        int pair = color_pairs[p];
        attron(COLOR_PAIR(pair));
        for (int i = 0; i < colors.at(pair).size(); i++) {
            ScreenChar screen_char = colors.at(pair)[i];
            mvaddch(screen_char.row, screen_char.col, screen_char.character.ch);
        }
        attroff(COLOR_PAIR(pair));
    }
}

