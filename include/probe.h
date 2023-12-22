
#ifndef TMEDIA_PROBE_H
#define TMEDIA_PROBE_H

#include "avguard.h"

#include "boiler.h"

extern "C" {
  #include <libavformat/avformat.h>
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

AVProbeFileRet av_probe_file(std::filesystem::path path);

std::optional<MediaType> media_type_probe(std::filesystem::path path);
bool probably_valid_media_file_path(std::filesystem::path path);
bool is_valid_media_file_path(std::filesystem::path path);

#endif