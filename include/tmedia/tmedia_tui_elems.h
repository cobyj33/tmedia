#ifndef TMEDIA_TMEDIA_TUI_ELEMS_H
#define TMEDIA_TMEDIA_TUI_ELEMS_H

class PixelData;
enum class ScalingAlgo;
enum class VidOutMode;

#include <string>
#include <vector>

extern "C" {
  #include <curses.h>
}

void render_pixel_data(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VidOutMode output_mode, const ScalingAlgo scaling_algorithm, std::string_view ascii_char_map);
void wprint_progress_bar(WINDOW* window, int y, int x, int width, int height, double percentage);
void wprint_playback_bar(WINDOW* window, int y, int x, int width, double time, double duration);
void wprint_labels(WINDOW* window, std::vector<std::string_view>& labels, int y, int x, int width);


void render_pixel_data_plain(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, const ScalingAlgo scaling_algorithm, std::string_view ascii_char_map);
void render_pixel_data_bg(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, const ScalingAlgo scaling_algorithm);
void render_pixel_data_color(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, const ScalingAlgo scaling_algorithm, std::string_view ascii_char_map);




#endif