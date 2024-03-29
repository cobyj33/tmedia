#include "ascii.h" 

#include "pixeldata.h"

#include <stdexcept>

extern "C" {
#include <libavutil/avutil.h>
}

const std::string ASCII_STANDARD_CHAR_MAP = "@%#*+=-:_.";

char get_char_from_value(const std::string& characters, uint8_t value) {
  return characters[ (std::size_t)value * (characters.length() - 1) / 255 ];
}

RGBColor get_rgb_from_char(const std::string& characters, char ch) {
  for (int i = 0; i < (int)characters.size(); i++) {
    if (ch == characters[i]) {
      int val = i * 255 / (characters.size() - 1); 
      return RGBColor(val);
    }
  }
  
  throw std::runtime_error("[get_rgb_from_char] Cannot get color value from char " +
  std::to_string(ch) + ", character \"" + std::to_string(ch) +
  "\" not found in string: " + characters);
}

char get_char_from_rgb(const std::string& characters, const RGBColor& color) {
  return get_char_from_value(characters, (uint8_t)get_grayscale(color.red, color.green, color.blue));
}


