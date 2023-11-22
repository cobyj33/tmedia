#ifndef TMEDIA_METADATA_H
#define TMEDIA_METADATA_H

#include <map>
#include <string>

extern "C" {
  #include <libavformat/avformat.h>
}

typedef std::map<std::string, std::map<std::string, std::string>> MetadataCache;

std::map<std::string, std::string> get_file_metadata(const std::string& file);
std::map<std::string, std::string> get_format_context_metadata(AVFormatContext* fmt_ctx);

void metadata_cache_cache(std::string file, MetadataCache& cache);
bool metadata_cache_has_file(std::string file, MetadataCache& cache);
bool metadata_cache_has(std::string file, std::string key, MetadataCache& cache);
std::string metadata_cache_get(std::string file, std::string key, MetadataCache& cache);

std::string get_media_file_display_name(std::string abs_path, MetadataCache& metadata_cache);

#endif