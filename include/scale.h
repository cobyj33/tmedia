#ifndef TMEDIA_IMAGE_H
#define TMEDIA_IMAGE_H
/**
 * @file scale.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Common functions for image manipulation in tmedia
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <utility>

struct VideoDimensions {
  int width;
  int height;
  VideoDimensions(int width, int height) : width(width), height(height) {}
  VideoDimensions() : width(0), height(0) {}
};


/**
 * @brief Returns a <width, height> pair representing the source dimensions bounded into the bounded dimensions while preserving aspect ratio
 * 
 * @note The source will NOT be expanded into the bounds. The bounds only serve as an area where the source cannot extend past. If the source already fits in the bounds, this function will perform no transformations and simply return the source dimensions through the pair.
 * @param src_width The width of the source frame
 * @param src_height The height of the source frame
 * @param max_width The width of the bounded box which the source will be fitted into
 * @param max_height The height of the bounded box which the source will be fitted into
 * @return The size of the frame when it is bounded into the bounded dimensions. 
 */
VideoDimensions get_bounded_dimensions(int src_width, int src_height, int max_width, int max_height);
VideoDimensions get_scale_size(int src_width, int src_height, int target_width, int target_height);
double get_scale_factor(int src_width, int src_height, int target_width, int target_height);
#endif
