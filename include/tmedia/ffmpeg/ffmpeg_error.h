#ifndef TMEDIA_EXCEPT_H
#define TMEDIA_EXCEPT_H

#include <stdexcept>

/**
 * I'm not sure how nicely ffmpeg_error will handle NOMEM errors tbh...
*/


#define FFMPEG_ERROR_STRING_SIZE 256

class ffmpeg_error : public std::runtime_error {
  public:
    int averror;
    char errstr[FFMPEG_ERROR_STRING_SIZE];
    /**
     * @brief Construct a new ffmpeg error object
     * Note: The error should already be passed through the AVERROR FFmpeg Macro
     * 
     * @param message The explanation of this error's occourance
     * @param averror The FFmpeg error code
     */
    ffmpeg_error(const std::string& message, int averror);
};

#endif