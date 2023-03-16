#ifndef ASCII_VIDEO_IMAGE
#define ASCII_VIDEO_IMAGE
/**
 * @file scale.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Common functions for image manipulation in ascii_video
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <utility>

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
std::pair<int, int> get_bounded_dimensions(int src_width, int src_height, int max_width, int max_height);
std::pair<int, int> get_scale_size(int src_width, int src_height, int target_width, int target_height);
double get_scale_factor(int src_width, int src_height, int target_width, int target_height);
#endif
