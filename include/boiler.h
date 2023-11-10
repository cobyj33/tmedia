#ifndef TMEDIA_BOILER_H
#define TMEDIA_BOILER_H

#include <string>
#include <map>

extern "C" {
#include <libavformat/avformat.h>
}

enum class MediaType {
  VIDEO,
  AUDIO,
  IMAGE
};

std::string media_type_to_string(MediaType media_type);
MediaType media_type_from_avformat_context(AVFormatContext* format_context);

bool probably_valid_media_file_path(const std::string& path_str);
bool is_valid_media_file_path(const std::string& path_str);

/**
 * @brief Opens and returns an AVFormat Context
 * 
 * @note The returned AVFormatContext is guaranteed to be non-null
 * @note The returned AVFormatContext must be freed with avformat_close_input, not just avformat_free_context
 * @throws std::runtime_error if the format context could not be opened for the corresponding file name
 * @throws std::runtime_error if the stream information for the format context could not be found
 * 
 * @param file_path The path of the file to open from the current working directory
 * @return An AVFormatContext pointer representing the opened media file
 */
AVFormatContext* open_format_context(const std::string& file_path);

/**
 * @brief Dump a media file's metadata into standard output.
 * @param file_path The path of the file to open from the current working directory
 */
void dump_file_info(const std::string& file_path);

/**
 * @brief Return the duration of the media file
 * @param file_path The path of the file to open from the current working directory
 * 
 * @throws std::runtime_error if a format context could not be opened for the corresponding file name
 * @throws std::runtime_error if the stream information for the format context could not be found
 * @return The duration of the media file at the selected path in seconds
 */
double get_file_duration(const std::string& file_path);

std::map<std::string, std::string> get_format_context_metadata(AVFormatContext* fmt_ctx);

bool avformat_context_has_media_stream(AVFormatContext* format_context, enum AVMediaType media_type);

#endif
