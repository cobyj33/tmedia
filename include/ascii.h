#ifndef ASCII_VIDEO_ASCII
#define ASCII_VIDEO_ASCII
#include <cstdint>
#include <vector>
#include <string>
#include "color.h"
#include "pixeldata.h"

extern const std::string ASCII_STANDARD_CHAR_MAP;

char get_char_from_value(const std::string& characters, uint8_t value);
char get_char_from_rgb(const std::string& characters, const RGBColor& color);
RGBColor get_rgb_from_char(const std::string& characters, char ch);

#endif
