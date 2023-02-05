#include "image.h"
#include <utility>
#include <algorithm>

double get_scale_factor(int srcWidth, int srcHeight, int targetWidth, int targetHeight) {
    double width_scaler = (double)targetWidth / srcWidth; // > 1 if growing, < 1 if shrinking, ==1 if same
    double height_scaler = (double)targetHeight / srcHeight; // > 1 if growing, < 1 if shrinking, ==1 if same
    return std::min(width_scaler, height_scaler);
}

std::pair<int, int> get_scale_size(int srcWidth, int srcHeight, int targetWidth, int targetHeight) { 
    double scale_factor = get_scale_factor(srcWidth, srcHeight, targetWidth, targetHeight);
    int width = (int)(srcWidth * scale_factor);
    int height = (int)(srcHeight * scale_factor);
    return std::make_pair(width, height);
}

std::pair<int, int> get_bounded_dimensions(int srcWidth, int srcHeight, int maxWidth, int maxHeight) {
  if (srcWidth <= maxWidth && srcHeight <= maxHeight) {
    return std::make_pair(srcWidth, srcHeight);
  } else {
    return get_scale_size(srcWidth, srcHeight, maxWidth, maxHeight);
  }
}