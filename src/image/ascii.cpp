#include <tmedia/image/ascii.h> 

#include <tmedia/image/pixeldata.h>
#include <tmedia/util/defines.h>

#include <fmt/format.h>

#include <stdexcept>

extern "C" {
#include <libavutil/avutil.h>
}

const char* ASCII_STANDARD_CHAR_MAP = "@%#*+=-:_.";


RGB24 get_rgb_from_char(std::string_view characters, char ch) {
  for (std::string::size_type i = 0; i < characters.size(); i++) {
    if (ch == characters[i]) {
      return RGB24(static_cast<uint8_t>(i * 255 / (characters.size() - 1)));
    }
  }
  
  throw std::runtime_error(fmt::format("[{}] Cannot get color value from char {}"
  ", character \"{}\" not found in string {} ", FUNCDINFO, ch, ch, characters));
}
