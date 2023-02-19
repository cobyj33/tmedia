#ifndef ASCII_VIDEO_IMAGE
#define ASCII_VIDEO_IMAGE
/**
 * @file image.h
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
 * @brief Returns a <width, height> pair representing the <src_width, src_height> bounded into the <max_width, max_height> dimensions while preserving aspect ratio
 * 
 * @param src_width 
 * @param src_height 
 * @param max_width 
 * @param max_height 
 * @return std::pair<int, int> 
 */
std::pair<int, int> get_bounded_dimensions(int src_width, int src_height, int max_width, int max_height);
std::pair<int, int> get_scale_size(int src_width, int src_height, int target_width, int target_height);
double get_scale_factor(int src_width, int src_height, int target_width, int target_height);
#endif
