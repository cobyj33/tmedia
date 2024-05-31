#ifndef TMEDIA_TMEDIA_TUI_ELEMS_H
#define TMEDIA_TMEDIA_TUI_ELEMS_H

class PixelData;

#include <string>
#include <vector>

extern "C" {
  #include <curses.h>
}

void wprint_progress_bar(WINDOW* window, int y, int x, int width, int height, double percentage);
void wprint_playback_bar(WINDOW* window, int y, int x, int width, double time, double duration);
void wprint_labels(WINDOW* window, std::string_view* labels, std::size_t nb_labels, int y, int x, int width);
void wprint_labels_middle(WINDOW* window, std::string_view* labels, std::size_t nb_labels, int y, int x, int width);

void render_pixel_data_plain(PixelData& pixel_data, PixelData& scaling_buffer, int bounds_row, int bounds_col, int bounds_width, int bounds_height, std::string_view ascii_char_map);
void render_pixel_data_bg(PixelData& pixel_data, PixelData& scaling_buffer, int bounds_row, int bounds_col, int bounds_width, int bounds_height);
void render_pixel_data_color(PixelData& pixel_data, PixelData& scaling_buffer, int bounds_row, int bounds_col, int bounds_width, int bounds_height, std::string_view ascii_char_map);




#endif