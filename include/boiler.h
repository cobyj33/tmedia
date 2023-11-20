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

extern "C" {
#include <libavformat/avformat.h>
}

enum class MediaType {
  VIDEO,
  AUDIO,
  IMAGE
};

typedef std::map<std::string, std::map<std::string, std::string>> MetadataCache;

std::map<std::string, std::string> get_file_metadata(const std::string& file);
std::map<std::string, std::string> get_format_context_metadata(AVFormatContext* fmt_ctx);

void metadata_cache_cache(std::string file, MetadataCache& cache);
bool metadata_cache_has_file(std::string file, MetadataCache& cache);
bool metadata_cache_has(std::string file, std::string key, MetadataCache& cache);
std::string metadata_cache_get(std::string file, std::string key, MetadataCache& cache);

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
void dump_format_context(AVFormatContext* format_context);

/**
 * @brief Return the duration of the media file in seconds
 * @param file_path The path of the file to open from the current working directory
 * 
 * @throws std::runtime_error if a format context could not be opened for the corresponding file name
 * @throws std::runtime_error if the stream information for the format context could not be found
 * @return The duration of the media file at the selected path in seconds
 */
double get_file_duration(const std::string& file_path);


bool avformat_context_has_media_stream(AVFormatContext* format_context, enum AVMediaType media_type);

#endif
