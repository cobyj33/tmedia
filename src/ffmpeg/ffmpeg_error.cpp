#include <tmedia/ffmpeg/ffmpeg_error.h> 
 
#include <string>
#include <string_view>
#include <fmt/format.h>

extern "C" {
  #include <libavutil/error.h>
}

ffmpeg_error::ffmpeg_error(std::string_view message, int averror) : std::runtime_error::runtime_error("") {
  this->averror = averror;
  this->errstr[0] = '\0'; // ensure no buffer overflow
  if (averror == AVERROR(ENOMEM)) {
    static_cast<std::runtime_error&>(*this) = std::runtime_error(std::string(message)); // directly forward error message
  } else {
    int res = av_strerror(averror, this->errstr, FFMPEG_ERROR_STRING_SIZE);
    if (res == 0) {
      static_cast<std::runtime_error&>(*this) = std::runtime_error(fmt::format("{}: ( {}: {} )", message, this->averror, this->errstr));
    } else {
      static_cast<std::runtime_error&>(*this) = std::runtime_error(fmt::format("{}: ({}: (No FFmpeg Error Description Found) )", message, this->averror));
    }
  }
}
