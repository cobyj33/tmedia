#include "ascii.h" 

#include "pixeldata.h"

#include <stdexcept>

extern "C" {
#include <libavutil/avutil.h>
}

const std::string ASCII_STANDARD_CHAR_MAP = "@%#*+=-:_.";
const std::string ASCII_EXTENDED_CHAR_MAP = "@MBHENR#KWXDFPQASUZbdehx*8Gm&04LOVYkpq5Tagns69owz$CIu23Jcfry%%1v7l+it[]{}?j|()=~!-/<>\"^_';,:`.";
const std::string UNICODE_BOX_CHAR_MAP = "█▉▊▋▌▍▎▏";
const std::string UNICODE_STIPPLE_CHAR_MAP = "░▒▓";

char get_char_from_value(const std::string& characters, uint8_t value) {
  return characters[ (size_t)value * (characters.length() - 1) / 255 ];
}

RGBColor get_rgb_from_char(const std::string& characters, char ch) {
  for (int i = 0; i < (int)characters.size(); i++) {
    if (ch == characters[i]) {
      uint8_t val = i * 255 / (characters.size() - 1); 
      return RGBColor(val);
    }
  }
  
  throw std::runtime_error("Cannot get color value from char " + std::to_string(ch) + ", character \"" + std::to_string(ch) + "\" not found in string: " + characters);
}

char get_char_from_rgb(const std::string& characters, const RGBColor& color) {
  return get_char_from_value(characters, (uint8_t)get_grayscale(color.red, color.green, color.blue));
}


