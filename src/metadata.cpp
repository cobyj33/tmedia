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

void mchc_cache(std::string file, MetadataCache& cache) {
  if (cache.count(file) == 0) {
    cache[file] = get_file_metadata(file);
  }
}

bool mchc_has_file(std::string file, MetadataCache& cache) {
  return cache.count(file) == 1;
}

bool mchc_has(std::string file, std::string key, MetadataCache& cache) {
  if (cache.count(file) == 1) {
    if (cache[file].count(key) == 1) {
      return true;
    }
  }
  return false;
}

std::string mchc_get(std::string file, std::string key, MetadataCache& cache) {
  if (!mchc_has(file, key, cache)) {
    if (mchc_has_file(file, cache)) {
      throw std::runtime_error(fmt::format("[{}] could not find key (\"{}\") "
      "in file (\"{}\") in metadata cache", FUNCDINFO, key, file));
    }
    throw std::runtime_error(fmt::format("[{}] could not find file (\"{}\") in "
    "metadata cache", FUNCDINFO, file));
  }
  return cache[file][key];
}



std::map<std::string, std::string> get_file_metadata(std::filesystem::path path) {
  AVFormatContext* fmt_ctx = open_format_context(path);
  std::map<std::string, std::string> metadata = fmt_ctx_meta(fmt_ctx);
  avformat_close_input(&fmt_ctx);
  return metadata;
}

std::map<std::string, std::string> fmt_ctx_meta(AVFormatContext* fmt_ctx) {
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