
#ifndef TMEDIA_PROBE_H
#define TMEDIA_PROBE_H

#include <tmedia/media/mediatype.h>
#include <tmedia/ffmpeg/avguard.h>

extern "C" {
struct AVInputFormat;
}

#include <string>
#include <optional>
#include <filesystem>

struct AVProbeFileRet {
  #if AVFORMAT_CONST_AVIOFORMAT
  const AVInputFormat* avif;
  #else
  AVInputFormat* avif;
  #endif
  int score = 0;
};

AVProbeFileRet av_probe_file(const std::filesystem::path& path);
std::optional<MediaType> media_type_probe(const std::filesystem::path& path);
bool probably_valid_media_file_path(const std::filesystem::path& path);
bool is_valid_media_file_path(const std::filesystem::path& path);
bool has_media_fsext(const std::filesystem::path& path);
std::optional<MediaType> media_type_from_path(const std::filesystem::path& path);

#endif