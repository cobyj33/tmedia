#include "probe.h"

#include "ffmpeg_error.h"
#include "boiler.h"
#include "formatting.h"
#include <funcmac.h>

#include <string>
#include <stdexcept>
#include <filesystem>

#include <fmt/format.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avstring.h>
}


AVProbeFileRet av_probe_file(std::filesystem::path path) {
  AVProbeFileRet res;

  AVIOContext* avio_ctx;
  int ret = avio_open(&avio_ctx, path.c_str(), AVIO_FLAG_READ);
  if (ret < 0) {
    throw ffmpeg_error(fmt::format("[{}] avio_open failure", FUNCDINFO), ret);
  }

  #if AVFORMAT_CONST_AVIOFORMAT
  const AVInputFormat* avif = NULL;
  #else
  AVInputFormat* avif = NULL;
  #endif

  res.score = av_probe_input_buffer2(avio_ctx, &avif, path.c_str(), NULL, 0, 0);
  avio_close(avio_ctx);
  res.avif = avif;
 
  if (res.score < 0) {
    throw ffmpeg_error(fmt::format("[{}] av_probe_input_buffer2 failure",
    FUNCDINFO), res.score);
  }

  return res;
}

bool probably_valid_media_file_path(std::filesystem::path path) {
  const AVOutputFormat* fmt = av_guess_format(NULL, path.filename().c_str(), NULL);
  if (fmt != NULL) return true;
  return is_valid_media_file_path(path);
}

std::optional<MediaType> media_type_probe(std::filesystem::path path) {
  try {
    AVProbeFileRet pfret = av_probe_file(path);
    if (pfret.score > AVPROBE_SCORE_RETRY) {
      if (std::optional<MediaType> from_iformat = media_type_from_iformat(pfret.avif)) {
        return from_iformat;
      }
    }
  } catch (const std::runtime_error& e) {
    // no-op
  }

  try {
    AVFormatContext* fmt_ctx = open_format_context(path);
    MediaType media_type = media_type_from_avformat_context(fmt_ctx);
    avformat_close_input(&fmt_ctx);
    return media_type;
  } catch (const std::runtime_error& e) {
    // no-op
  }

  return std::nullopt;
}

bool is_valid_media_file_path(std::filesystem::path path) {
  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec)) return false;

  try {
    AVProbeFileRet pfret = av_probe_file(path);
    if (pfret.score > AVPROBE_SCORE_EXTENSION) return true;
  } catch (const std::runtime_error& e) {
    // no-op
  }

  try {
    AVFormatContext* fmt_ctx = open_format_context(path);
    avformat_close_input(&fmt_ctx);
    return true;
  } catch (const std::runtime_error& e) {
    return false;
  }

  return false;
}