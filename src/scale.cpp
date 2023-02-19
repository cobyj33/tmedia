#include <utility>
#include <algorithm>

#include "image.h"

double get_scale_factor(int src_width, int src_height, int target_width, int target_height) {
    double width_scaler = (double)target_width / src_width; // > 1 if growing, < 1 if shrinking, ==1 if same
    double height_scaler = (double)target_height / src_height; // > 1 if growing, < 1 if shrinking, ==1 if same
    return std::min(width_scaler, height_scaler);
}

std::pair<int, int> get_scale_size(int src_width, int src_height, int target_width, int target_height) { 
    double scale_factor = get_scale_factor(src_width, src_height, target_width, target_height);
    int width = (int)(src_width * scale_factor);
    int height = (int)(src_height * scale_factor);
    return std::make_pair(width, height);
}

std::pair<int, int> get_bounded_dimensions(int src_width, int src_height, int max_width, int max_height) {
  if (src_width <= max_width && src_height <= max_height) {
    return std::make_pair(src_width, src_height);
  } else {
    return get_scale_size(src_width, src_height, max_width, max_height);
  }
}