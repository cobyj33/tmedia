#include "metadata.h"

#include "formatting.h"
#include "boiler.h"
#include "funcmac.h"

#include <fmt/format.h>

#include <stdexcept>
#include <map>
#include <string>

extern "C" {
  #include <libavformat/avformat.h>
}

void mchc_cache(const std::string& file, MetadataCache& cache) {
  if (cache.count(file) == 0) {
    cache[file] = get_file_metadata(file);
  }
}

bool mchc_has_file(std::string_view file, MetadataCache& cache) {
  return cache.count(file) == 1;
}

bool mchc_has(const std::string& file, std::string_view key, MetadataCache& cache) {
  if (cache.count(file) == 1) {
    return cache[file].count(key) == 1;
  }
  return false;
}

std::string_view mchc_get(const std::string& file, const std::string& key, MetadataCache& cache) {
  return cache[file][key];
}



Metadata get_file_metadata(const std::filesystem::path& path) {
  AVFormatContext* fmt_ctx = open_format_context(path);
  Metadata metadata = fmt_ctx_meta(fmt_ctx);
  avformat_close_input(&fmt_ctx);
  return metadata;
}

Metadata fmt_ctx_meta(AVFormatContext* fmt_ctx) {
  Metadata dict;

  for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
    const AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(fmt_ctx->streams[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
      dict[tag->key] = tag->value;
    }
  }

  const AVDictionaryEntry *tag = NULL;  // any main file metadata overrides any specific stream metadata
  while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
    dict[tag->key] = tag->value;
  }

  return dict;
}