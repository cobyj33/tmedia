#include <tmedia/util/defines.h> 

#include <tmedia/image/pixeldata.h>
#include <tmedia/util/defines.h>

#include <fmt/format.h>

#include <stdexcept>

extern "C" {
#include <libavutil/avutil.h>
}

const char* ASCII_STANDARD_CHAR_MAP = "@%#*+=-:_.";

char get_char_from_value(std::string_view characters, std::uint8_t value) {
  return characters[ (std::size_t)value * (characters.length() - 1) / 255UL ];
}

RGB24 get_rgb_from_char(std::string_view characters, char ch) {
  for (std::string::size_type i = 0; i < characters.size(); i++) {
    if (ch == characters[i]) {
      return RGB24(static_cast<uint8_t>(i * 255 / (characters.size() - 1)));
    }
  }
  
  throw std::runtime_error(fmt::format("[{}] Cannot get color value from char {}"
  ", character \"{}\" not found in string {} ", FUNCDINFO, ch, ch, characters));
}

char get_char_from_rgb(std::string_view characters, RGB24 color) {
  return get_char_from_value(characters, get_gray8(color.r, color.g, color.b));
}


