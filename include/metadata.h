#ifndef TMEDIA_METADATA_H
#define TMEDIA_METADATA_H

#include <map>
#include <string>
#include <filesystem>

extern "C" {
  #include <libavformat/avformat.h>
}

typedef std::map<std::string, std::string> Metadata;

Metadata get_file_metadata(std::filesystem::path path);
Metadata fmt_ctx_meta(AVFormatContext* fmt_ctx);

/**
 * A general cache-type map between filenames or some other media id strings
 * and metadata maps.
*/
typedef std::map<std::string, Metadata> MetadataCache;

void mchc_cache(std::string file, MetadataCache& cache);

/**
 * Check if the given metadata cache contains a file for
*/
bool mchc_has_file(std::string file, MetadataCache& cache);
bool mchc_has(std::string file, std::string key, MetadataCache& cache);
std::string mchc_get(std::string file, std::string key, MetadataCache& cache);


#endif