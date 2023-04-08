#ifndef ASCII_VIDEO_BOILER
#define ASCII_VIDEO_BOILER

#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

/**
 * @brief Opens and returns an AVFormat Context
 * 
 * @note The returned AVFormatContext is guaranteed to be non-null
 * @throws std::runtime_error if the format context could not be opened for the corresponding file name
 * @throws std::runtime_error if the stream information for the format context could not be found
 * 
 * @param file_path The path of the file to open from the current working directory
 * @return An AVFormatContext pointer representing the opened media file
 */
AVFormatContext* open_format_context(std::string file_path);

/**
 * @brief Dump a media file's metadata into standard output.
 * @param file_path The path of the file to open from the current working directory
 */
void dump_file_info(const char* file_path);

/**
 * @brief Return the duration of the media file
 * @param file_path The path of the file to open from the current working directory
 * 
 * @throws std::runtime_error if a format context could not be opened for the corresponding file name
 * @throws std::runtime_error if the stream information for the format context could not be found
 * @return The duration of the media file at the selected path in seconds
 */
double get_file_duration(const char* file_path);

bool avformat_context_is_static_image(AVFormatContext* format_context);
bool avformat_context_is_video(AVFormatContext* format_context);
bool avformat_context_has_media_stream(AVFormatContext* format_context, enum AVMediaType media_type);

#endif
