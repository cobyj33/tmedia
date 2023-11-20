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

extern const std::string ASCII_STANDARD_CHAR_MAP;

char get_char_from_value(const std::string& characters, uint8_t value);
char get_char_from_rgb(const std::string& characters, const RGBColor& color);
RGBColor get_rgb_from_char(const std::string& characters, char ch);

#endif
