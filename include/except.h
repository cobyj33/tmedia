#ifndef ASCII_VIDEO_EXCEPT_INCLUDE
#define ASCII_VIDEO_EXCEPT_INCLUDE

#include <stdexcept>

extern "C" {
    #include <libavutil/error.h>
}


namespace ascii {
    class ffmpeg_error : public std::runtime_error {
        private:
            int averror;
            std::string error_string;
            std::string message;
        
        public:
            /**
             * @brief Construct a new ffmpeg error object
             * Note: The error should already be passed through the AVERROR FFmpeg Macro (basically, try to keep it negative)
             * 
             * @param message The explanation of this error's occourance
             * @param averror The FFmpeg error code
             */
            ffmpeg_error(const std::string message, int averror) : std::runtime_error::runtime_error("") {
                char errBuf[512];
                av_strerror(averror, errBuf, 512);
                this->averror = averror;
                this->error_string = std::string(errBuf);
                this->message = message;
                static_cast<std::runtime_error&>(*this) = std::runtime_error(this->message + ": " + this->error_string + " (" + std::to_string(this->averror) + " ).");
            }

            int get_averror() const {
                return this->averror;
            }

            std::string get_averror_string() const {
                return this->error_string;
            }
    };
}

#endif