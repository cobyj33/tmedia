
#include "boiler.h"

#include "decode.h"
#include "except.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <filesystem>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/log.h>
}

AVFormatContext* open_format_context(const std::string& file_path) {
  AVFormatContext* format_context = nullptr;
  int result = avformat_open_input(&format_context, file_path.c_str(), nullptr, nullptr);
  if (result < 0) {
    if (format_context != nullptr) {
      avformat_close_input(&format_context);
    }
    throw ascii::ffmpeg_error("Failed to open format context input for " + file_path, result);
  }

  result = avformat_find_stream_info(format_context, NULL);
  if (result < 0) {
    if (format_context != nullptr) {
      avformat_close_input(&format_context);
    }
    throw ascii::ffmpeg_error("Failed to find stream info for " + file_path, result);
  }

  if (format_context != nullptr) {
    return format_context;
  }
  throw std::runtime_error("Failed to open format context input, unknown error occured");
}

void dump_file_info(const std::string& file_path) {
  const int saved_avlog_level = av_log_get_level();
  av_log_set_level(AV_LOG_INFO);
  AVFormatContext* format_context = open_format_context(file_path);
  av_dump_format(format_context, 0, file_path.c_str(), 0);
  avformat_close_input(&format_context);
  av_log_set_level(saved_avlog_level);
}

double get_file_duration(const std::string& file_path) {
  AVFormatContext* format_context = open_format_context(file_path);
  int64_t duration = format_context->duration;
  double duration_seconds = (double)duration / AV_TIME_BASE;
  avformat_close_input(&format_context);
  return duration_seconds;
}

bool avformat_context_has_media_stream(AVFormatContext* format_context, enum AVMediaType media_type) {
  for (unsigned int i = 0; i < format_context->nb_streams; i++) {
    if (format_context->streams[i]->codecpar->codec_type == media_type) {
      return true;
    }
  }
  return false;
}

bool is_valid_media_file_path(const std::string& path_str) {
  std::filesystem::path path(path_str);
  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec)) return false;

  try {
    AVFormatContext* fmt_ctx = open_format_context(path_str);
    avformat_close_input(&fmt_ctx);
    return true;
  } catch (const ascii::ffmpeg_error& e) {
    return false;
  }

  return false;
}
