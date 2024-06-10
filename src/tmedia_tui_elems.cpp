#include <tmedia/tmedia_tui_elems.h>

#include <tmedia/image/ascii.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/util/defines.h>
#include <tmedia/util/formatting.h>
#include <tmedia/tmcurses/tmcurses.h>
#include <tmedia/image/scale.h>

#include <fmt/format.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <cassert>
#include <algorithm>

extern "C" {
  #include <curses.h>
}

/**
 * 
 * 
 * @returns A reference to the PixelData instance which contains the bounded
 * src image. If the source image is already bounded within the given dimensions,
 * then a reference to the source image will be returned, and the buf PixelData
 * instance will not be modified. If the source image
 * must be bounded inside of the given dimensions, then "buf" will be altered
 * to contain the bounded image data and a reference to buf will be returned.
 * 
 * NOTE:
 * width and height can be equal to or less than 0. If width or height are
 * equal to or less than 0, then buf is modified to have a width and height
 * of 0 and a reference to buf is returned.
*/
PixelData& pixdata_bound(PixelData& buf, PixelData& src, int width, int height) {
  // handle invalid dimensions case. This allows calling code to not have
  // to worry about width and height being non-negative
  if (width <= 0 || height <= 0) {
    pixdata_initgray(buf, 0, 0, 0);
    return buf;
  }

  // quick exit if the source already fits into the bounds
  if (src.m_width <= width && src.m_height <= height) {
    return src;
  }

  const double scale_factor = get_scale_factor(src.m_width, src.m_height, width, height);
  const int new_width = src.m_width * scale_factor;
  const int new_height = src.m_height * scale_factor;
  pixdata_setnewdims(buf, new_width, new_height);
  
  // Simple Nearest-Neighbor algorithm.
  // Since we render at such low resolutions anyway, I believe the performance
  // gain of nearest neighbor makes up from its slightly worse results
  // compared to an algorithm that would have to iterate over the entire
  // source such as Box Sampling. Perhaps a Bilinear scaling algorithm would
  // be interesting to implement later.

  std::size_t pi = 0;
  for (double new_row = 0; new_row < new_height; new_row++) {
    for (double new_col = 0; new_col < new_width; new_col++) {
      buf.pixels[pi++] = src.pixels[(int)(new_row / scale_factor) * src.m_width + (int)(new_col / scale_factor)];
    }
  }

  return buf;
}

void wprint_progress_bar(WINDOW* window, int y, int x, int width, int height, double percentage) {
  const int passed_width = width * percentage;
  const int remaining_width = width * (1.0 - percentage);
  wfill_box(window, y, x, passed_width, height, '@');
  wfill_box(window, y, x + passed_width, remaining_width, height, '-');
}

void wprint_playback_bar(WINDOW* window, int y, int x, int width, double time_in_seconds, double duration_in_seconds) {
  static constexpr int PADDING_BETWEEN_ELEMENTS = 2;

  const std::string formatted_passed_time = format_duration(time_in_seconds);
  const std::string formatted_duration = format_duration(duration_in_seconds);
  const std::string current_time_string = fmt::format("{} / {}", format_duration(time_in_seconds), format_duration(duration_in_seconds));
  TMLabelStyle duration_label_style(y, x, width, TMAlign::LEFT, 0, 0);
  tm_mvwaddstr_label(window, duration_label_style, current_time_string);

  const int progress_bar_width = width - current_time_string.length() - PADDING_BETWEEN_ELEMENTS;

  if (progress_bar_width > 0)
    wprint_progress_bar(window, y, x + current_time_string.length() + PADDING_BETWEEN_ELEMENTS, progress_bar_width, 1,time_in_seconds / duration_in_seconds);
}

void render_pixel_data_plain(PixelData& pix_data, PixelData& scaling_buffer, int bounds_row, int bounds_col, int bounds_width, int bounds_height, std::string_view ascii_char_map) {
  PixelData& bounded = pixdata_bound(scaling_buffer, pix_data, bounds_width, bounds_height);
  const int image_start_row = bounds_row + std::abs(bounded.m_height - bounds_height) / 2;
  const int image_start_col = bounds_col + std::abs(bounded.m_width - bounds_width) / 2;

  std::size_t pi = 0;
  for (int row = 0; row < bounded.m_height; row++) {
    move(image_start_row + row, image_start_col);
    for (int col = 0; col < bounded.m_width; col++) {
      addch(get_char_from_rgb(ascii_char_map, bounded.pixels[pi++]));
    }
  }
}

void render_pixel_data_bg(PixelData& pix_data, PixelData& scaling_buffer, int bounds_row, int bounds_col, int bounds_width, int bounds_height) {
  PixelData& bounded = pixdata_bound(scaling_buffer, pix_data, bounds_width, bounds_height);
  const int image_start_row = bounds_row + std::abs(bounded.m_height - bounds_height) / 2;
  const int image_start_col = bounds_col + std::abs(bounded.m_width - bounds_width) / 2;

  std::size_t pi = 0;
  for (int row = 0; row < bounded.m_height; row++) {
    move(image_start_row + row, image_start_col);
    for (int col = 0; col < bounded.m_width; col++) {
      const int color_pair = get_closest_tmcurses_color_pair(bounded.pixels[pi++]);
      addch(' ' | COLOR_PAIR(color_pair));
    }
  }
}

void render_pixel_data_color(PixelData& pix_data, PixelData& scaling_buffer, int bounds_row, int bounds_col, int bounds_width, int bounds_height, std::string_view ascii_char_map) {
  PixelData& bounded = pixdata_bound(scaling_buffer, pix_data, bounds_width, bounds_height);
  const int image_start_row = bounds_row + std::abs(bounded.m_height - bounds_height) / 2;
  const int image_start_col = bounds_col + std::abs(bounded.m_width - bounds_width) / 2;

  std::size_t pi = 0;
  for (int row = 0; row < bounded.m_height; row++) {
    move(image_start_row + row, image_start_col);
    for (int col = 0; col < bounded.m_width; col++) {
      const RGB24 target_color = bounded.pixels[pi++];
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
  const int real_y = std::clamp<int>(req_y, 0, LINES - 1);

  const int start_x = std::clamp<int>(req_center - (real_len / 2), 0, COLS - 1);
  const int end_x = std::clamp<int>(start_x + real_len, 0, COLS - 1);

  int walk_x = start_x;
  for (std::size_t i = 0; i < nb_labels && walk_x <= end_x; i++) {
    const int width_remaining = end_x - walk_x;
    TMLabelStyle label_style(real_y, walk_x, width_remaining, TMAlign::LEFT, 0, 0);
    tm_mvwaddstr_label(window, label_style, labels[i]);
    walk_x += static_cast<int>(labels[i].length());
  }
}
