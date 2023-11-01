#ifndef TMEDIA_TMEDIA_RENDER_H
#define TMEDIA_TMEDIA_RENDER_H

#include "pixeldata.h"
#include "tmedia_vom.h"

#include <string>
#include <vector>

extern "C" {
  #include <curses.h>
}


void render_pixel_data(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode, const ScalingAlgo scaling_algorithm, const std::string& ascii_char_map);
void wprint_progress_bar(WINDOW* window, int y, int x, int width, int height, double percentage);
void wprint_playback_bar(WINDOW* window, int y, int x, int width, double time, double duration);
void wprint_labels(WINDOW* window, std::vector<std::string>& labels, int y, int x, int width);


#endif