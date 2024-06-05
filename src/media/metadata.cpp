#include <tmedia/media/metadata.h>

#include <tmedia/util/formatting.h>
#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/util/defines.h>

#include <fmt/format.h>

#include <string>

extern "C" {
  #include <libavformat/avformat.h>
}

void mchc_cache_file(const std::filesystem::path& file, MetadataCache& cache) {
  if (mchc_has_file(file.c_str(), cache)) return;

  MetadataCacheEntry cache_entry{ std::string(file), {} };
  std::unique_ptr<AVFormatContext, AVFormatContextDeleter> fmt_ctx = open_fctx(file);
  fmt_ctx_meta(fmt_ctx.get(), cache_entry.value);
  cache.push_back(std::move(cache_entry));
}

bool mchc_has_file(std::string_view file, MetadataCache& cache) {
  for (std::size_t i = 0; i < cache.size(); i++) {
    if (cache[i].key == file) return true;
  }
  return false;
}

std::string_view mchc_add(std::string_view file, std::string_view key, std::string_view val, MetadataCache& cache) {
  for (std::size_t i = 0; i < cache.size(); i++) {
    if (cache[i].key == file) {
      return metadata_add(key, val, cache[i].value);
    }
  }

  cache.push_back({ std::string(file), {} });
  return metadata_add(key, val, cache[cache.size() - 1].value);
}


std::optional<std::string_view> mchc_get(std::string_view file, std::string_view key, MetadataCache& cache) {
  for (std::size_t i = 0; i < cache.size(); i++) {
    if (cache[i].key == file) {
      return metadata_get(key, cache[i].value);
    }
  }

  return std::nullopt;
}


std::string_view metadata_add(std::string_view key, std::string_view val, Metadata& meta) {
  for (std::size_t j = 0; j < meta.size(); j++) {
    if (meta[j].key == key) {
      meta[j].value = std::string(val);
      return meta[j].value;
    }
  }

  // no metadata entry for [key] found, add an entry
  meta.push_back({ std::string(key), std::string(val) });
  return meta[meta.size() - 1].value;
}

std::optional<std::string_view> metadata_get(std::string_view key, Metadata& meta) {
  for (std::size_t i = 0; i < meta.size(); i++) {
    if (meta[i].key == key) {
      return meta[i].value;
    }
  }

  return std::nullopt;
}

void fmt_ctx_meta(AVFormatContext* fmt_ctx, Metadata& outdict) {
  for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
    const AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(fmt_ctx->streams[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
      (void)metadata_add(tag->key, tag->value, outdict);
    }
  }

  const AVDictionaryEntry *tag = NULL;  // any main file metadata overrides any specific stream metadata
  while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
    (void)metadata_add(tag->key, tag->value, outdict);
  }
}