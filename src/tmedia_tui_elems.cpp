#include <tmedia/tmedia_tui_elems.h>

#include <tmedia/image/ascii.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/util/defines.h>
#include <tmedia/util/formatting.h>
#include <tmedia/tmcurses/tmcurses.h>
#include <tmedia/util/defines.h>
#include <tmedia/util/wmath.h>
#include <tmedia/image/scale.h>

#include <fmt/format.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <cassert>

extern "C" {
  #include <curses.h>
}

RGB24 get_avg_color_from_area(const PixelData& pixel_data, double _row, double _col, double _width, double _height) {
  const int row = static_cast<int>(std::floor(_row));
  const int col = static_cast<int>(std::floor(_col));
  const int width = static_cast<int>(std::ceil(_width));
  const int height = static_cast<int>(std::ceil(_height));

  assert(width * height > 0);
  assert(row >= 0);
  assert(col >= 0);


  const int rowmax = std::min(pixel_data.m_width, row + height);
  const int colmax = std::min(pixel_data.m_height, col + width);
  int sums[3] = {0, 0, 0};
  int colors = 0;

  for (int curr_row = row; curr_row < rowmax; curr_row++) {
    for (int curr_col = col; curr_col < colmax; curr_col++) {
      RGB24 color = pixel_data.pixels[curr_row * pixel_data.m_width + curr_col];
      sums[0] += color.r;
      sums[1] += color.g;
      sums[2] += color.b;
      colors++;
    }
  }

  colors = (colors == 0) ? 1 : colors; // prevent divide by 0
  return RGB24((sums[0]/colors) & 0xFF, (sums[1]/colors) & 0xFF, (sums[2]/colors) & 0xFF);
}

void pixdata_scale(PixelData& dest, PixelData& src, double amount) {
  assert(amount >= 0);
  if (amount == 0) {
    pixdata_setnewdims(dest, 0, 0);
    return;
  }

  const int new_width = src.m_width * amount;
  const int new_height = src.m_height * amount;
  pixdata_setnewdims(dest, new_width, new_height);
  std::size_t pi = 0;
  for (double new_row = 0; new_row < new_height; new_row++) {
    for (double new_col = 0; new_col < new_width; new_col++) {
      dest.pixels[pi++] = src.pixels[(int)(new_row / amount) * src.m_width + (int)(new_col / amount)];
    }
  }
}

PixelData& pixdata_bound(PixelData& buf, PixelData& src, int width, int height) {
  if (src.m_width <= width && src.m_height <= height) {
    return src;
  }

  double scale_factor = get_scale_factor(src.m_width, src.m_height, width, height);
  pixdata_scale(buf, src, scale_factor);
  return buf;
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