#include <tmedia/ffmpeg/ffmpeg_error.h> 
 
#include <string>
#include <fmt/format.h>

extern "C" {
  #include <libavutil/error.h>
}

ffmpeg_error::ffmpeg_error(const std::string& message, int averror) : std::runtime_error::runtime_error("") {
  this->averror = averror;
  if (averror == AVERROR(ENOMEM)) {
    static_cast<std::runtime_error&>(*this) = std::runtime_error(message); // directly forward error message
  } else {
    int res = av_strerror(averror, this->errstr, FFMPEG_ERROR_STRING_SIZE);
    if (res == 0) {
      static_cast<std::runtime_error&>(*this) = std::runtime_error(fmt::format("{}: ( {}: {} )", message, this->averror, this->errstr));
    } else {
      static_cast<std::runtime_error&>(*this) = std::runtime_error(fmt::format("{}: ({})", message, this->averror));
    }
  }
}
