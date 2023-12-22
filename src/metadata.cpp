#include "metadata.h"

#include "formatting.h"
#include "boiler.h"

#include <stdexcept>
#include <map>
#include <string>

extern "C" {
  #include <libavformat/avformat.h>
}

void metadata_cache_cache(std::string file, MetadataCache& cache) {
  if (cache.count(file) == 0) {
    cache[file] = get_file_metadata(file);
  }
}

bool metadata_cache_has_file(std::string file, MetadataCache& cache) {
  return cache.count(file) == 1;
}

bool metadata_cache_has(std::string file, std::string key, MetadataCache& cache) {
  if (cache.count(file) == 1) {
    if (cache[file].count(key) == 1) {
      return true;
    }
  }
  return false;
}

std::string metadata_cache_get(std::string file, std::string key, MetadataCache& cache) {
  if (!metadata_cache_has(file, key, cache)) {
    if (metadata_cache_has_file(file, cache)) {
      throw std::runtime_error("[metadata_cache_get] could not find key " + key + " in file " + file + " in metadata cache");
    }
    throw std::runtime_error("[metadata_cache_get] could not find file " + file + " in metadata cache");
  }
  return cache[file][key];
}

std::string get_media_file_display_name(std::string abs_path, MetadataCache& metadata_cache) {
  metadata_cache_cache(abs_path, metadata_cache);
  bool has_artist = metadata_cache_has(abs_path, "artist", metadata_cache);
  bool has_title = metadata_cache_has(abs_path, "title", metadata_cache);

  if (has_artist && has_title) {
    return metadata_cache_get(abs_path, "artist", metadata_cache) + " - " + metadata_cache_get(abs_path, "title", metadata_cache);
  } else if (has_title) {
    return metadata_cache_get(abs_path, "title", metadata_cache);
  }
  return to_filename(abs_path);
}

std::map<std::string, std::string> get_file_metadata(std::filesystem::path path) {
  AVFormatContext* fmt_ctx = open_format_context(path);
  std::map<std::string, std::string> metadata = get_format_context_metadata(fmt_ctx);
  avformat_close_input(&fmt_ctx);
  return metadata;
}

std::map<std::string, std::string> get_format_context_metadata(AVFormatContext* fmt_ctx) {
  std::map<std::string, std::string> dict;

  std::vector<AVDictionary*> metadata_dicts;
  for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
    metadata_dicts.push_back(fmt_ctx->streams[i]->metadata);
  }
  metadata_dicts.push_back(fmt_ctx->metadata); // any main file metadata overrides any specific stream metadata

  for (const AVDictionary* avdict : metadata_dicts) {
    const AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(avdict, "", tag, AV_DICT_IGNORE_SUFFIX))) {
      dict[tag->key] = tag->value;
    }
  }

  return dict;
}