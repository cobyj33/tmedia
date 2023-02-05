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
 * @brief Returns a <width, height> pair representing the <srcWidth, srcHeight> bounded into the <maxWidth, maxHeight> dimensions while preserving aspect ratio
 * 
 * @param srcWidth 
 * @param srcHeight 
 * @param maxWidth 
 * @param maxHeight 
 * @return std::pair<int, int> 
 */
std::pair<int, int> get_bounded_dimensions(int srcWidth, int srcHeight, int maxWidth, int maxHeight);
std::pair<int, int> get_scale_size(int srcWidth, int srcHeight, int targetWidth, int targetHeight);
double get_scale_factor(int srcWidth, int srcHeight, int targetWidth, int targetHeight);
#endif
