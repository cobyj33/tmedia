#ifndef TMEDIA_ASCII_H
#define TMEDIA_ASCII_H

#include "color.h"
#include "pixeldata.h"

#include <cstdint>
#include <vector>
#include <string>

/**
 * @file ascii.h
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

extern const std::string ASCII_STANDARD_CHAR_MAP;

/**
 * Returns a character from a character
*/
char get_char_from_value(const std::string& characters, uint8_t value);
char get_char_from_rgb(const std::string& characters, const RGBColor& color);

/**
 * 
*/
RGBColor get_rgb_from_char(const std::string& characters, char ch);

#endif
