
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

#include "gui.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

extern "C" {
#include "ncurses.h"
#include <libavutil/avutil.h>
}

const int KEY_ESCAPE = 27;

// void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode);
// void print_pixel_data_text(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height);

void print_pixel_data(PixelData& pixel_data, ftxui::Screen& screen, VideoOutputMode output_mode);
void print_pixel_data_text(PixelData& pixel_data, ftxui::Screen& screen);

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

    double batched_jump_time = 0;
    const int RENDER_LOOP_SLEEP_TIME_MS = 41;
    std::unique_lock<std::mutex> lock(alter_mutex, std::defer_lock);
    ftxui::Screen screen = ftxui::Screen::Create(ftxui::Dimension::Full(), ftxui::Dimension::Full());
    screen.SetCursor({0, 0, ftxui::Screen::Cursor::Hidden });


    // std::unique_lock<std::mutex> lock(alter_mutex, std::defer_lock);
    erase();

    while (player->in_use) {

        int input = wgetch(inputWindow);

        lock.lock();
        if (input == ERR) { // no input coming in, simply render the current image, sleep, and restart
            goto render;
        }


        if (player->playback.get_time(system_clock_sec()) >= player->get_duration()) {
            player->in_use = false;
            break;
        }

        while (input != ERR) { // Go through and process all the batched input
            if (input == KEY_ESCAPE || input == KEY_BACKSPACE) {
                player->in_use = false;
                break;
            }

            // if (input == KEY_RESIZE) {
            //     erase();
            //     endwin();
            //     refresh();
            // }




            if (input == KEY_LEFT || input == KEY_RIGHT) {
                if (input == KEY_LEFT) {
                    batched_jump_time -= 5;
                } else if (input == KEY_RIGHT) {
                    batched_jump_time += 5;
                }
                printw("%s%s%s\n", "Preparing to Jump ", std::to_string(batched_jump_time).c_str(), " seconds...");
            }

            if (input == ' ') {
                player->playback.toggle(system_clock_sec());
            }

            input = wgetch(inputWindow);
        }

        if (batched_jump_time != 0) {
            double current_playback_time = player->playback.get_time(system_clock_sec());
            printw("%s%s%s\n", "Jumping", std::to_string(batched_jump_time).c_str(), " seconds...");
            double target_time = current_playback_time + batched_jump_time;
            target_time = clamp(target_time, 0.0, player->get_duration());
            player->jump_to_time(target_time, system_clock_sec());
            batched_jump_time = 0;
        }

        render:
            PixelData& image = player->cache.image;
            lock.unlock();

            print_pixel_data(image, screen, gui_state.get_video_output_mode());

            std::cout << screen.ToString() << screen.ResetPosition();
            screen.Clear();
            sleep_for_ms(41);
    }

    delwin(inputWindow);

    // while (player->in_use) {
        // lock.lock();

        // if (player->playback.get_time(system_clock_sec()) >= player->get_duration()) {
        //     player->in_use = false;
        //     lock.unlock();
        //     break;
        // }

        // PixelData& image = player->cache.image;
        // lock.unlock();

        // print_pixel_data(image, screen, gui_state.get_video_output_mode());

        // std::cout << screen.ToString() << screen.ResetPosition();
        // screen.Clear();
        // sleep_for_ms(41);
    // }
}


void print_pixel_data_text(PixelData& pixel_data, ftxui::Screen& screen) {

    PixelData bounded = pixel_data.bound(screen.dimx(), screen.dimy());
    const AsciiImage image(bounded, AsciiImage::ASCII_STANDARD_CHAR_MAP);
    int image_start_row = std::abs(image.get_height() - screen.dimy()) / 2;
    int image_start_col = std::abs(image.get_width() - screen.dimx()) / 2; 


    for (int row = 0; row < image.get_height(); row++) {
        for (int col = 0; col < image.get_width(); col++) {
            ftxui::Pixel& pixel = screen.PixelAt(image_start_col + col, image_start_row + row);
            pixel.character = image.at(row, col).ch;
        } 
    }

}


void print_pixel_data(PixelData& pixel_data, ftxui::Screen& screen, VideoOutputMode output_mode) {
    if (output_mode == VideoOutputMode::TEXT_ONLY) {
        print_pixel_data_text(pixel_data, screen);
        return;
    }

    bool grayscale = output_mode == VideoOutputMode::GRAYSCALE || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;
    bool background_only = output_mode == VideoOutputMode::COLORED_BACKGROUND_ONLY || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;

    const int bounds_width = screen.dimx();
    const int bounds_height = screen.dimy();

    PixelData bounded = pixel_data.bound(screen.dimx(), screen.dimy());
    const AsciiImage image(bounded, AsciiImage::ASCII_STANDARD_CHAR_MAP);
    int image_start_row = std::abs(image.get_height() - screen.dimy()) / 2;
    int image_start_col = std::abs(image.get_width() - screen.dimx()) / 2; 


    for (int row = 0; row < image.get_height(); row++) {
        for (int col = 0; col < image.get_width(); col++) {
            ftxui::Pixel& pixel = screen.PixelAt(image_start_col + col, image_start_row + row);
            ColorChar colorChar = image.at(row, col);
            pixel.character = background_only ? ' ' : colorChar.ch;
            RGBColor color = grayscale ? colorChar.color.create_grayscale() : colorChar.color;
            pixel.background_color = ftxui::Color(color.red & 0xff, color.green & 0xff,  color.blue & 0xff);
        } 
    }

}

