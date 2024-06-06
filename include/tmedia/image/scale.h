#ifndef TMEDIA_IMAGE_H
#define TMEDIA_IMAGE_H
/**
 * @file tmedia/image/scale.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Common functions for determining scaling dimensions and factors
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 */

struct Dim2 {
  int width;
  int height;

  [[gnu::always_inline]] constexpr bool operator==(Dim2 o) {
    return this->width == o.width && this->height == o.height;
  }

  [[gnu::always_inline]] constexpr bool operator!=(Dim2 o) {
    return this->width != o.width || this->height != o.height;
  }
};

/**
 * Note that all of these scaling and bounding functions aim to preserve
 * the aspect ratio of the provided source dimensions
*/

/**
 * Returns a Dim2 representing the source dimensions bounded into the bounded dimensions while preserving aspect ratio.
 * 
 * The source will NOT be expanded into the bounds. The bounds only serve as
 * an area where the source cannot extend past. If the source already fits
 * in the bounds, this function will perform no transformations and simpl
 *  return the given source dimensions.
 * 
 * @param src_width The width of the source frame
 * @param src_height The height of the source frame
 * @param max_width The width of the bounded box which the source will be fitted into
 * @param max_height The height of the bounded box which the source will be fitted into
 * @return The size of the frame when it is bounded into the bounded dimensions. 
 */
constexpr inline double get_scale_factor(int src_width, int src_height, int target_width, int target_height) {
  double width_scaler = static_cast<double>(target_width) / src_width; // > 1 if growing, < 1 if shrinking, ==1 if same
  double height_scaler = static_cast<double>(target_height) / src_height; // > 1 if growing, < 1 if shrinking, ==1 if same
  return width_scaler < height_scaler ? width_scaler : height_scaler;
}

/**
 * Returns a Dim2 representing the source dimensions bounded into the bounded dimensions while preserving aspect ratio.
 * 
 * The source will NOT be expanded into the bounds. The bounds only serve as
 * an area where the source cannot extend past. If the source already fits
 * in the bounds, this function will perform no transformations and simpl
 *  return the given source dimensions.
 * 
 * @param src_width The width of the source frame
 * @param src_height The height of the source frame
 * @param max_width The width of the bounded box which the source will be fitted into
 * @param max_height The height of the bounded box which the source will be fitted into
 * @return The size of the frame when it is bounded into the bounded dimensions. 
 */
constexpr inline Dim2 get_scale_size(int src_width, int src_height, int target_width, int target_height) { 
  const double scale_factor = get_scale_factor(src_width, src_height, target_width, target_height);
  const int width = static_cast<int>(src_width * scale_factor);
  const int height = static_cast<int>(src_height * scale_factor);
  return { width, height };
}


/**
 * Returns a Dim2 representing the source dimensions bounded into the bounded dimensions while preserving aspect ratio.
 * 
 * The source will NOT be expanded into the bounds. The bounds only serve as
 * an area where the source cannot extend past. If the source already fits
 * in the bounds, this function will perform no transformations and simpl
 *  return the given source dimensions.
 * 
 * @param src_width The width of the source frame
 * @param src_height The height of the source frame
 * @param max_width The width of the bounded box which the source will be fitted into
 * @param max_height The height of the bounded box which the source will be fitted into
 * @return The size of the frame when it is bounded into the bounded dimensions. 
 */
constexpr inline Dim2 bound_dims(int src_width, int src_height, int max_width, int max_height) {
  if (src_width <= max_width && src_height <= max_height) {
    return { src_width, src_height };
  } else {
    return get_scale_size(src_width, src_height, max_width, max_height);
  }
}

#endif
