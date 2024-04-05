#include "tmedia_tui_elems.h"

#include "pixeldata.h"
#include "ascii.h"
#include "formatting.h"
#include "tmcurses.h"
#include "tmedia_vom.h"
#include "funcmac.h"

#include <fmt/format.h>


#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
  #include <curses.h>
}


void wprint_progress_bar(WINDOW* window, int y, int x, int width, int height, double percentage) {
  const int passed_width = width * percentage;
  const int remaining_width = width * (1.0 - percentage);
  wfill_box(window, y, x, passed_width, height, '@');
  wfill_box(window, y, x + passed_width, remaining_width, height, '-');
}

void wprint_playback_bar(WINDOW* window, int y, int x, int width, double time_in_seconds, double duration_in_seconds) {
  constexpr int PADDING_BETWEEN_ELEMENTS = 2;

  const std::string formatted_passed_time = format_duration(time_in_seconds);
  const std::string formatted_duration = format_duration(duration_in_seconds);
  const std::string current_time_string = formatted_passed_time + " / " + formatted_duration;
  TMLabelStyle duration_label_style(y, x, width, TMAlign::LEFT, 0, 0);
  tm_mvwaddstr_label(window, duration_label_style, current_time_string.c_str());

  const int progress_bar_width = width - current_time_string.length() - PADDING_BETWEEN_ELEMENTS;

  if (progress_bar_width > 0) 
    wprint_progress_bar(window, y, x + current_time_string.length() + PADDING_BETWEEN_ELEMENTS, progress_bar_width, 1,time_in_seconds / duration_in_seconds);
}

void render_pixel_data(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VidOutMode output_mode, const ScalingAlgo scaling_algorithm, const std::string& ascii_char_map) {
  if (output_mode != VidOutMode::PLAIN && !has_colors()) {
    throw std::runtime_error(fmt::format("[{}] Attempted to print colored text "
    "in terminal that does not support color", FUNCDINFO));
  }

  PixelData bounded = pixel_data.bound(bounds_width, bounds_height, scaling_algorithm);
  int image_start_row = bounds_row + std::abs(bounded.get_height() - bounds_height) / 2;
  int image_start_col = bounds_col + std::abs(bounded.get_width() - bounds_width) / 2; 
  bool background_only = output_mode == VidOutMode::COLOR_BG || output_mode == VidOutMode::GRAY_BG;

  for (int row = 0; row < bounded.get_height(); row++) {
    for (int col = 0; col < bounded.get_width(); col++) {
      const RGB24& target_color = bounded.at(row, col);
      const char target_char = background_only ? ' ' : get_char_from_rgb(ascii_char_map, target_color);
      
      if (output_mode == VidOutMode::PLAIN) {
        mvaddch(image_start_row + row, image_start_col + col, target_char);
      } else {
        const int color_pair = get_closest_ncurses_color_pair(target_color);
        mvaddch(image_start_row + row, image_start_col + col, target_char | COLOR_PAIR(color_pair));
      }
    }
  }
}

void wprint_labels(WINDOW* window, std::vector<std::string>& labels, int y, int x, int width) {
  if (width < 0)
    throw std::runtime_error(fmt::format("[{}] attempted to print to negative width space", FUNCDINFO));
  if (width == 0)
    return;

  int section_size = width / labels.size();
  for (std::size_t i = 0; i < labels.size(); i++) {
    TMLabelStyle label_style(y, x + section_size * static_cast<int>(i), section_size, TMAlign::CENTER, 0, 0);
    tm_mvwaddstr_label(window, label_style, labels[i].c_str());
  }
}