#ifndef ASCII_VIDEO_EXCEPT_INCLUDE
#define ASCII_VIDEO_EXCEPT_INCLUDE

#include <stdexcept>

extern "C" {
    #include <libavutil/error.h>
}


namespace ascii {
    class bad_alloc : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class not_found_error : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class forbidden_action_error : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class ffmpeg_error : public std::runtime_error {
        int averror;
        std::string error_string;
        std::string message;

        inline const char* what() const noexcept {
            return (this->message + ": " + this->error_string + " (" + std::to_string(this->averror) + " ).").c_str(); 
        }

        /**
         * @brief Construct a new ffmpeg error object
         * Note: The error should already be passed through the AVERROR FFmpeg Macro (basically, try to keep it negative)
         * 
         * @param message 
         * @param averror 
         */
        public:
            ffmpeg_error(const std::string message, int averror) : std::runtime_error::runtime_error(message) {
                this->averror = averror;
                char errBuf[512];
                av_strerror(averror, errBuf, 512);
                this->error_string = std::string(errBuf);
                this->message = message;
            }

            int get_averror() const {
                return this->averror;
            }

            bool is_eagain() const {
                return this->averror == AVERROR(EAGAIN);
            }
    };
}

#endif