
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <map>
#include <iostream>

#include "audio.h"
#include "color.h"
#include "pixeldata.h"
#include "playheadlist.hpp"
#include "scale.h"
#include "ascii.h"
#include "icons.h"
#include "media.h"
#include "threads.h"
#include "wmath.h"
#include "wtime.h"
#include "termcolor.h"

#include "gui.h"

extern "C"
{
#include "ncurses.h"
#include <libavutil/avutil.h>
}

const int KEY_ESCAPE = 27;

void render_movie_screen(PixelData& pixel_data, VideoOutputMode gui_state);
void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode);
void print_pixel_data_text(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height);

RGBColor get_index_display_color(int index, int length)
{
    const double step = (255.0 / 2.0) / length;
    const double color_space_size = 255 - (step * (double)(index / 6));
    int red = index % 6 >= 3 ? color_space_size : 0;
    int green = index % 2 == 0 ? color_space_size : 0;
    int blue = index % 6 < 3 ? color_space_size : 0;
    return RGBColor(red, green, blue);
}

void render_loop(MediaPlayer *player, std::mutex &alter_mutex, GUIState gui_state)
{
    WINDOW *inputWindow = newwin(0, 0, 1, 1);
    nodelay(inputWindow, true);
    keypad(inputWindow, true);

    double batched_jump_time = 0;
    const int RENDER_LOOP_SLEEP_TIME_MS = 41;
    std::unique_lock<std::mutex> lock(alter_mutex, std::defer_lock);
    erase();

    while (player->in_use)
    {

        int input = wgetch(inputWindow);

        lock.lock();
        if (input == ERR)
        { // no input coming in, simply render the current image, sleep, and restart loop
            goto render;
        }

        if (player->get_time(system_clock_sec()) >= player->get_duration())
        {
            player->in_use = false;
            break;
        }

        while (input != ERR)
        { // Go through and process all the batched input
            if (input == KEY_ESCAPE || input == KEY_BACKSPACE)
            {
                player->in_use = false;
                break;
            }

            if (input == KEY_RESIZE)
            {
                erase();
                refresh();
            }

            if (input == KEY_LEFT || input == KEY_RIGHT)
            {
                if (input == KEY_LEFT)
                {
                    batched_jump_time -= 5;
                }
                else if (input == KEY_RIGHT)
                {
                    batched_jump_time += 5;
                }
            }

            if (input == ' ')
            {
                player->playback.toggle(system_clock_sec());
            }

            input = wgetch(inputWindow);
        }

        if (batched_jump_time != 0)
        {
            double current_playback_time = player->get_time(system_clock_sec());
            double target_time = current_playback_time + batched_jump_time;
            target_time = clamp(target_time, 0.0, player->get_duration());
            player->jump_to_time(target_time, system_clock_sec());
            batched_jump_time = 0;
        }

    render:
        PixelData &image = player->cache.image;
        lock.unlock();

        render_movie_screen(image, gui_state.get_video_output_mode());
        refresh();
        sleep_for_ms(41);
    }

    delwin(inputWindow);
}

void render_movie_screen(PixelData& pixel_data, VideoOutputMode output_mode) {
    print_pixel_data(pixel_data, 0, 0, COLS, LINES, output_mode);
}

void print_pixel_data_text(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height) {
    PixelData bounded = pixel_data.bound(bounds_width, bounds_height);
    const AsciiImage image(bounded, AsciiImage::ASCII_STANDARD_CHAR_MAP);
    int image_start_row = bounds_row + std::abs(image.get_height() - bounds_height) / 2;
    int image_start_col = bounds_col + std::abs(image.get_width() - bounds_width) / 2; 

    for (int row = 0; row < image.get_height(); row++) {
        for (int col = 0; col < image.get_width(); col++) {
            mvaddch(image_start_row + row, image_start_col + col, image.at(row, col).ch);
        } 
    }
}


void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode) {
    if (output_mode == VideoOutputMode::TEXT_ONLY) {
        print_pixel_data_text(pixel_data, bounds_row, bounds_col, bounds_width, bounds_height);
        return;
    }
    
    if (!has_colors()) {
        throw std::runtime_error("Attempted to print colored text in terminal that does not support color");
    }

    PixelData bounded = pixel_data.bound(bounds_width, bounds_height);
    const AsciiImage image(bounded, AsciiImage::ASCII_STANDARD_CHAR_MAP);
    int image_start_row = bounds_row + std::abs(image.get_height() - bounds_height) / 2;
    int image_start_col = bounds_col + std::abs(image.get_width() - bounds_width) / 2; 

    bool grayscale = output_mode == VideoOutputMode::GRAYSCALE || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;
    bool background_only = output_mode == VideoOutputMode::COLORED_BACKGROUND_ONLY || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;

    for (int row = 0; row < image.get_height(); row++) {
        for (int col = 0; col < image.get_width(); col++) {
            ColorChar color_char = image.at(row, col);
            color_char = ColorChar(background_only ? ' ' : color_char.ch, color_char.color);
            int color_pair = grayscale ? get_closest_ncurses_grayscale_color_pair(color_char.color) : get_closest_ncurses_color_pair(color_char.color);
            mvaddch(image_start_row + row, image_start_col + col, color_char.ch | COLOR_PAIR(color_pair));
        }
    }
}