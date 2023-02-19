#ifndef ASCII_VIDEO_BOILER
#define ASCII_VIDEO_BOILER

#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

/**
 * @brief Opens and returns an AVFormat Context
 * 
 * Note that it is guaranteed to not be NULL or nullptr
 * 
 * @param file_name 
 * @return AVFormatContext* 
 */
AVFormatContext* open_format_context(std::string file_name);

#endif
