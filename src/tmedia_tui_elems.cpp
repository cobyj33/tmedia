#include <tmedia/tmedia_tui_elems.h>

#include <tmedia/image/ascii.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/util/defines.h>
#include <tmedia/util/formatting.h>
#include <tmedia/tmcurses/tmcurses.h>
#include <tmedia/util/defines.h>
#include <tmedia/util/wmath.h>

#include <fmt/format.h>


#include <stdexcept>
#include <string>
#include <vector>
#include <cassert>

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
  const std::string current_time_string = fmt::format("{} / {}", format_duration(time_in_seconds), format_duration(duration_in_seconds));
  TMLabelStyle duration_label_style(y, x, width, TMAlign::LEFT, 0, 0);
  tm_mvwaddstr_label(window, duration_label_style, current_time_string);

  const int progress_bar_width = width - current_time_string.length() - PADDING_BETWEEN_ELEMENTS;

  if (progress_bar_width > 0) 
    wprint_progress_bar(window, y, x + current_time_string.length() + PADDING_BETWEEN_ELEMENTS, progress_bar_width, 1,time_in_seconds / duration_in_seconds);
}

void render_pixel_data_plain(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, const ScalingAlgo scaling_algorithm, std::string_view ascii_char_map) {
  const PixelData bounded = pixel_data.bound(bounds_width, bounds_height, scaling_algorithm);
  const int image_start_row = bounds_row + std::abs(bounded.get_height() - bounds_height) / 2;
  const int image_start_col = bounds_col + std::abs(bounded.get_width() - bounds_width) / 2; 

  const std::vector<RGB24>& pixels = bounded.data();
  const int width = bounded.get_width(); 

  for (int row = 0; row < bounded.get_height(); row++) {
    move(image_start_row + row, image_start_col);
    const int row_offset = row * width;
    for (int col = 0; col < width; col++) {
      addch(get_char_from_rgb(ascii_char_map, pixels[row_offset + col]));
    }
  }
}

void render_pixel_data_bg(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, const ScalingAlgo scaling_algorithm) {
  const PixelData bounded = pixel_data.bound(bounds_width, bounds_height, scaling_algorithm);
  const int image_start_row = bounds_row + std::abs(bounded.get_height() - bounds_height) / 2;
  const int image_start_col = bounds_col + std::abs(bounded.get_width() - bounds_width) / 2; 
  const std::vector<RGB24>& pixels = bounded.data();
  const int width = bounded.get_width();

  for (int row = 0; row < bounded.get_height(); row++) {
    move(image_start_row + row, image_start_col);
    const int row_offset = row * width;
    for (int col = 0; col < width; col++) {
      const int color_pair = get_closest_tmcurses_color_pair(pixels[row_offset + col]);
      addch(' ' | COLOR_PAIR(color_pair));
    }
  }
}

void render_pixel_data_color(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, const ScalingAlgo scaling_algorithm, std::string_view ascii_char_map) {
  const PixelData bounded = pixel_data.bound(bounds_width, bounds_height, scaling_algorithm);
  const int image_start_row = bounds_row + std::abs(bounded.get_height() - bounds_height) / 2;
  const int image_start_col = bounds_col + std::abs(bounded.get_width() - bounds_width) / 2; 
  const std::vector<RGB24>& pixels = bounded.data();
  const int width = bounded.get_width();

  for (int row = 0; row < bounded.get_height(); row++) {
    move(image_start_row + row, image_start_col);
    const int row_offset = row * width;
    for (int col = 0; col < width; col++) {
      const RGB24 target_color = pixels[row_offset + col];
      const char target_char = get_char_from_rgb(ascii_char_map, target_color);
      const int color_pair = get_closest_tmcurses_color_pair(target_color);
      addch(target_char | COLOR_PAIR(color_pair));
    }
  }
}

void wprint_labels(WINDOW* window, std::string_view* labels, std::size_t nb_labels, int y, int x, int width) {
  if (width <= 0)
    return;

  int section_size = width / nb_labels;
  for (std::size_t i = 0; i < nb_labels; i++) {
    TMLabelStyle label_style(y, x + section_size * static_cast<int>(i), section_size, TMAlign::CENTER, 0, 0);
    tm_mvwaddstr_label(window, label_style, labels[i]);
  }
}

void wprint_labels_middle(WINDOW* window, std::string_view* labels, std::size_t nb_labels, int req_y, int req_x, int req_width) {
  if (req_width <= 0) return;

  int labels_len = 0;
  for (std::size_t i = 0; i < nb_labels; i++) {
    labels_len += static_cast<int>(labels[i].length());
  }

  const int real_len = std::min(req_width, labels_len);
  if (real_len <= 0) return;


  // center is defined as the center provided by the user
  const int req_center = req_x + (req_width / 2);
  const int real_y = clamp(req_y, 0, LINES - 1);

  const int start_x = clamp(req_center - (real_len / 2), 0, COLS - 1);
  const int end_x = clamp(start_x + real_len, 0, COLS - 1);

  int walk_x = start_x;
  for (std::size_t i = 0; i < nb_labels && walk_x <= end_x; i++) {
    const int width_remaining = end_x - walk_x;
    TMLabelStyle label_style(real_y, walk_x, width_remaining, TMAlign::LEFT, 0, 0);
    tm_mvwaddstr_label(window, label_style, labels[i]);
    walk_x += static_cast<int>(labels[i].length());
  }
}