#ifndef TMEDIA_EXCEPT_H
#define TMEDIA_EXCEPT_H

#include <stdexcept>
#include <string_view>


/**
 * The size of the errstr character array of the ffmpeg_error class.
 */
#define FFMPEG_ERROR_STRING_SIZE 256

/**
 * std::runtime_error made to contain an FFmpeg averror and error string as
 * members to be propogated up the call stack.
 *
 * NOTE:
 * Errors will be formatted in the format "{message}: ( {averror}: {errstr} )",
 * where "{message}" is the string message given to ffmpeg_error on
 * construction, "{averror}" is the integer value of the FFmpeg error passed
 * to the ffmpeg_error constructor and "{errstr}" is the string representation
 * of the passed in averror. This means that any message given to ffmpeg_error
 * should not end with a colon, as ffmpeg_error will automatically format
 * the given message to have a colon after it.
 *
 * NOTE:
 * In the current implementation, if averror == AVERROR(ENOMEM), then
 * the passed in string will simply be forwarded directly into errstr.
*/
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
    ffmpeg_error(std::string_view message, int averror);
};

#endif
