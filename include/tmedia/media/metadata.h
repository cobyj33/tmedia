#ifndef TMEDIA_METADATA_H
#define TMEDIA_METADATA_H

#include <map>
#include <string>
#include <filesystem>
#include <string_view>

extern "C" {
  #include <libavformat/avformat.h>
}

typedef std::map<std::string, std::string, std::less<>> Metadata;

Metadata get_file_metadata(const std::filesystem::path& path);
Metadata fmt_ctx_meta(AVFormatContext* fmt_ctx);

/**
 * A general cache-type map between filenames or some other media id strings
 * and metadata maps.
*/
typedef std::map<std::string, Metadata, std::less<>> MetadataCache;

void mchc_cache(const std::string& file, MetadataCache& cache);

/**
 * Check if the given metadata cache contains a file for
*/
bool mchc_has_file(std::string_view file, MetadataCache& cache);
bool mchc_has(const std::string& file, std::string_view key, MetadataCache& cache);
std::string_view mchc_get(const std::string& file, const std::string& key, MetadataCache& cache);


#endif