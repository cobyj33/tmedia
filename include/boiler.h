#ifndef TMEDIA_BOILER_H
#define TMEDIA_BOILER_H

/**
 * @file boiler.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Boilerplate reducing functions for various miscellaneous FFmpeg operations
 * @version 0.1
 * @date 2023-11-19
 * 
 * @copyright Copyright (c) 2023
 */

#include <string>
#include <map>
#include <optional>

#include <filesystem>

extern "C" {
#include <libavformat/avformat.h>
}

enum class MediaType {
  VIDEO,
  AUDIO,
  IMAGE
};

std::string media_type_to_string(MediaType media_type);
MediaType media_type_from_avformat_context(AVFormatContext* fmt_ctx);
std::optional<MediaType> media_type_from_mime_type(std::string_view mime_type);
std::optional<MediaType> media_type_from_iformat(const AVInputFormat* iformat);

/**
 * @brief Opens and returns an AVFormat Context
 * 
 * @note The returned AVFormatContext is guaranteed to be non-null
 * @note The returned AVFormatContext must be freed with avformat_close_input, not just avformat_free_context
 * @throws std::runtime_error if the format context could not be opened for the corresponding file name
 * @throws std::runtime_error if the stream information for the format context could not be found
 * 
 * @param path The path of the file to open from the current working directory
 * @return An AVFormatContext pointer representing the opened media file
 */
AVFormatContext* open_format_context(const std::filesystem::path& path);

/**
 * @brief Dump a media file's metadata into standard output.
 * @param path The path of the file to open from the current working directory
 */
void dump_file_info(const std::filesystem::path& path);
void dump_format_context(AVFormatContext* fmt_ctx);

/**
 * @brief Return the duration of the media file in seconds
 * @param path The path of the file to open from the current working directory
 * 
 * @throws std::runtime_error if a format context could not be opened for the corresponding file name
 * @throws std::runtime_error if the stream information for the format context could not be found
 * @return The duration of the media file at the selected path in seconds
 */
double get_file_duration(const std::filesystem::path& path);


bool avformat_context_has_media_stream(AVFormatContext* fmt_ctx, enum AVMediaType media_type);

#endif
