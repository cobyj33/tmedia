#ifndef TMEDIA_METADATA_H
#define TMEDIA_METADATA_H

#include <string>
#include <filesystem>
#include <string_view>
#include <vector>
#include <optional>

extern "C" {
  #include <libavformat/avformat.h>
}

struct MetadataEntry {
  std::string key;
  std::string value;
};

typedef std::vector<MetadataEntry> Metadata;

void fmt_ctx_meta(AVFormatContext* fmt_ctx, Metadata& outdict);
std::string_view metadata_add(std::string_view key, std::string_view value, Metadata& meta);
std::optional<std::string_view> metadata_get(std::string_view key, Metadata& meta);

struct MetadataCacheEntry {
  std::string key;
  std::vector<MetadataEntry> value;
};

/**
 * A general cache-type map between filenames or some other media id strings
 * and metadata maps.
*/
typedef std::vector<MetadataCacheEntry> MetadataCache;

void mchc_cache_file(const std::filesystem::path& file, MetadataCache& cache);
std::string_view mchc_add(std::string_view file, std::string_view key, std::string_view val, MetadataCache& cache);

/**
 * Check if the given metadata cache contains a file for
*/
bool mchc_has_file(std::string_view file, MetadataCache& cache);
std::optional<std::string_view> mchc_get(std::string_view file, std::string_view key, MetadataCache& cache);


#endif