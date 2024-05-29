#ifndef TMEDIA_ASCII_H
#define TMEDIA_ASCII_H

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>

#include <tmedia/util/defines.h>
#include <tmedia/image/color.h>

/**
 * @file tmedia/image/defines.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Common functions for converting from colors to ascii characters
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 */

/**
 * A character map for ASCII values is a simple linear ramp from black to white
 * of what each grayscale value will be mapped to. For example, the string
 * "@#-_" will have "_" as representing the lightest and "@" as representing the darkest,
 * with "#" and "-" representing gray values in between.
 * 
*/

extern const char* ASCII_STANDARD_CHAR_MAP;

TMEDIA_ALWAYS_INLINE inline constexpr char get_char_from_value(std::string_view characters, std::uint8_t value) {
  return characters[ (std::size_t)value * (characters.length() - 1) / 255UL ];
}

TMEDIA_ALWAYS_INLINE inline constexpr char get_char_from_rgb(std::string_view characters, RGB24 color) {
  return get_char_from_value(characters, get_gray8(color.r, color.g, color.b));
}

/**
 * 
*/
RGB24 get_rgb_from_char(std::string_view characters, char ch);

#endif
