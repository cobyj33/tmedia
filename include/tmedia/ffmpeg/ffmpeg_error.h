#ifndef TMEDIA_EXCEPT_H
#define TMEDIA_EXCEPT_H

#include <stdexcept>
#include <string>

#include <fmt/format.h>
#include <tmedia/util/defines.h>

extern "C" {
  #include <libavutil/error.h>
}

/**
 * I'm not sure how nicely ffmpeg_error will handle nomem errors tbh...
*/

std::string av_strerror_string(int errnum);

#define FFMPEG_ERROR_STRING_SIZE 256

class ffmpeg_error : public std::runtime_error {
  private:
    int averror;
    char errstr[FFMPEG_ERROR_STRING_SIZE];
  
  public:
    /**
     * @brief Construct a new ffmpeg error object
     * Note: The error should already be passed through the AVERROR FFmpeg Macro (basically, try to keep it negative)
     * 
     * @param message The explanation of this error's occourance
     * @param averror The FFmpeg error code
     */
    ffmpeg_error(const std::string& message, int averror) : std::runtime_error::runtime_error("") {
      this->averror = averror;
      if (averror == AVERROR(ENOMEM)) {
        static_cast<std::runtime_error&>(*this) = std::runtime_error(message); // directly forward error message
      } else {
        int found_desc = av_strerror(averror, this->errstr, FFMPEG_ERROR_STRING_SIZE);
        if (found_desc) {
          static_cast<std::runtime_error&>(*this) = std::runtime_error(fmt::format("{}: ( {}: {} )", message, this->averror, this->errstr));
        } else {
          static_cast<std::runtime_error&>(*this) = std::runtime_error(fmt::format("{}: ({})", message, this->averror));
        }
      }
    }

    TMEDIA_ALWAYS_INLINE inline int get_averror() const {
      return this->averror;
    }

    TMEDIA_ALWAYS_INLINE inline std::string get_averror_string() const {
      return std::string(this->errstr);
    }
};

#endif