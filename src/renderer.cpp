
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
#include "scale.h"
#include "ascii.h"
#include "media.h"
#include "sleep.h"
#include "wmath.h"
#include "wtime.h"
#include "avcurses.h"
#include "sigexit.h"

#include "gui.h"

extern "C"
{
#include "curses.h"
#include <libavutil/avutil.h>
}

const int KEY_ESCAPE = 27;
const double VOLUME_CHANGE_AMOUNT = 0.05;

void render_movie_screen(PixelData& pixel_data, VideoOutputMode media_gui);
void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode);

RGBColor get_index_display_color(int index, int length)
{
  const double step = (255.0 / 2.0) / length;
  const double color_space_size = 255 - (step * (double)(index / 6));
  int red = index % 6 >= 3 ? color_space_size : 0;
  int green = index % 2 == 0 ? color_space_size : 0;
  int blue = index % 6 < 3 ? color_space_size : 0;
  return RGBColor(red, green, blue);
}

void MediaPlayer::render_loop()
{
  WINDOW *inputWindow = newwin(0, 0, 1, 1);
  if (inputWindow == NULL) {
    this->in_use = false;
    return;
  }

  nodelay(inputWindow, true);
  keypad(inputWindow, true);
  double batched_jump_time = 0;
  const int RENDER_LOOP_SLEEP_TIME_MS = 41;
  erase();
  try {

    while (this->in_use) {
      PixelData image;
      VideoOutputMode vom;
      this->display_lines = LINES;
      this->display_cols = COLS;

      {
        std::lock_guard<std::mutex> lock(this->alter_mutex);

        if (should_sig_exit()) { // lmao i know this is bad ok I'll fix it later
          this->in_use = false;
          break;
        }

        int input = wgetch(inputWindow);


        if (this->media_type != MediaType::IMAGE && this->get_time(system_clock_sec()) >= this->get_duration()) {
          if (this->is_looped) {
            this->jump_to_time(0.0, system_clock_sec());
          } else {   
            this->in_use = false;
            break;
          }
        }

        while (input != ERR) { // Go through and process all the batched input
          if (input == KEY_ESCAPE || input == KEY_BACKSPACE || input == 127 || input == '\b') {
            this->in_use = false;
            break; // break out of input != ERR
          }

          if (input == KEY_RESIZE) {
            erase();
            refresh();
          }

          if ((input == 'c' || input == 'C') && has_colors() && can_change_color()) { // Change from current video mode to colored version
            switch (this->media_gui.get_video_output_mode()) {
              case VideoOutputMode::COLORED:
                this->media_gui.set_video_output_mode(VideoOutputMode::TEXT_ONLY);
                break;
              case VideoOutputMode::GRAYSCALE:
                this->media_gui.set_video_output_mode(VideoOutputMode::COLORED);
                ncurses_set_color_palette(AVNCursesColorPalette::RGB);
                break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY:
                this->media_gui.set_video_output_mode(VideoOutputMode::TEXT_ONLY);
                break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: 
                this->media_gui.set_video_output_mode(VideoOutputMode::COLORED_BACKGROUND_ONLY);
                ncurses_set_color_palette(AVNCursesColorPalette::RGB);
                break; 
              case VideoOutputMode::TEXT_ONLY:
                this->media_gui.set_video_output_mode(VideoOutputMode::COLORED);
                ncurses_set_color_palette(AVNCursesColorPalette::RGB);
                break;
            }
          }

          if ((input == 'g' || input == 'G') && has_colors() && can_change_color()) {
            switch (this->media_gui.get_video_output_mode()) {
              case VideoOutputMode::COLORED:
                this->media_gui.set_video_output_mode(VideoOutputMode::GRAYSCALE);
                ncurses_set_color_palette(AVNCursesColorPalette::GRAYSCALE);
                break;
              case VideoOutputMode::GRAYSCALE:
                this->media_gui.set_video_output_mode(VideoOutputMode::TEXT_ONLY);
                break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY:
                this->media_gui.set_video_output_mode(VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY);
                ncurses_set_color_palette(AVNCursesColorPalette::GRAYSCALE);
                break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: 
                this->media_gui.set_video_output_mode(VideoOutputMode::TEXT_ONLY);
                break; 
              case VideoOutputMode::TEXT_ONLY:
                this->media_gui.set_video_output_mode(VideoOutputMode::GRAYSCALE);
                ncurses_set_color_palette(AVNCursesColorPalette::GRAYSCALE);
                break;
            }
          }

          if ((input == 'b' || input == 'B') && has_colors() && can_change_color()) {
            switch (this->media_gui.get_video_output_mode()) {
              case VideoOutputMode::COLORED:
                this->media_gui.set_video_output_mode(VideoOutputMode::COLORED_BACKGROUND_ONLY);
                break;
              case VideoOutputMode::GRAYSCALE:
                this->media_gui.set_video_output_mode(VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY);
                break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY:
                this->media_gui.set_video_output_mode(VideoOutputMode::COLORED);
                break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: 
                this->media_gui.set_video_output_mode(VideoOutputMode::GRAYSCALE);
                break; 
              case VideoOutputMode::TEXT_ONLY: break;
            }
          }

          if ((input == KEY_LEFT || input == KEY_RIGHT) && (this->media_type == MediaType::VIDEO || this->media_type == MediaType::AUDIO)) {
            if (input == KEY_LEFT) {
              batched_jump_time -= 5;
            }
            else if (input == KEY_RIGHT) {
              batched_jump_time += 5;
            }
          }

          if (input == KEY_UP) {
            this->volume = clamp(this->volume + VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
          }

          if (input == KEY_DOWN) {
            this->volume = clamp(this->volume - VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
          }

          if (input == ' ' && (this->media_type == MediaType::VIDEO || this->media_type == MediaType::AUDIO)) {
            this->clock.toggle(system_clock_sec());
          }

          if (input == 'm' || input == 'M') {
            this->muted = !this->muted;
          }

          input = wgetch(inputWindow);
        } // Ending of "while (input != ERR)"

        if (batched_jump_time != 0 && (this->media_type == MediaType::VIDEO || this->media_type == MediaType::AUDIO)) {
          double current_playback_time = this->get_time(system_clock_sec());
          double target_time = current_playback_time + batched_jump_time;

          if (this->is_looped) {
            target_time = target_time < 0 ? this->get_duration() + std::fmod(target_time, this->get_duration()) : std::fmod(target_time, this->get_duration());
          } else {
            target_time = clamp(target_time, 0.0, this->get_duration());
          }

          this->jump_to_time(target_time, system_clock_sec());
          batched_jump_time = 0;
        }


        image = this->frame;
        vom = this->media_gui.get_video_output_mode();
      }

      render_movie_screen(image, vom);
      refresh();
      sleep_for_ms(RENDER_LOOP_SLEEP_TIME_MS);
    }

  } catch (std::exception const& e) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->error = std::string(e.what());
    this->in_use = false;
  }

  delwin(inputWindow);
}

void render_movie_screen(PixelData& pixel_data, VideoOutputMode output_mode) {
  print_pixel_data(pixel_data, 0, 0, COLS, LINES, output_mode);
}


void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode) {
  if (output_mode != VideoOutputMode::TEXT_ONLY && !has_colors()) {
    throw std::runtime_error("Attempted to print colored text in terminal that does not support color");
  }

  PixelData bounded = pixel_data.bound(bounds_width, bounds_height);
  int image_start_row = bounds_row + std::abs(bounded.get_height() - bounds_height) / 2;
  int image_start_col = bounds_col + std::abs(bounded.get_width() - bounds_width) / 2; 

  bool background_only = output_mode == VideoOutputMode::COLORED_BACKGROUND_ONLY || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;

  for (int row = 0; row < bounded.get_height(); row++) {
    for (int col = 0; col < bounded.get_width(); col++) {
      const RGBColor& target_color = bounded.at(row, col);
      const char target_char = background_only ? ' ' : get_char_from_rgb(AsciiImage::ASCII_STANDARD_CHAR_MAP, target_color);
      
      if (output_mode == VideoOutputMode::TEXT_ONLY) {
        mvaddch(image_start_row + row, image_start_col + col, target_char);
      } else {
        const int color_pair = get_closest_ncurses_color_pair(target_color);
        mvaddch(image_start_row + row, image_start_col + col, target_char | COLOR_PAIR(color_pair));
      }

    }
  }

}