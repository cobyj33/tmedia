
#include "boiler.h"

#include "decode.h"
#include "except.h"

#include <stdexcept>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/log.h>
}

AVFormatContext* open_format_context(std::string file_path) {
  AVFormatContext* format_context = nullptr;
  int result = avformat_open_input(&format_context, file_path.c_str(), nullptr, nullptr);
  if (result < 0) {
    if (format_context != nullptr) {
      avformat_free_context(format_context);
    }
    throw ascii::ffmpeg_error("Failed to open format context input for " + file_path, result);
  }

  result = avformat_find_stream_info(format_context, NULL);
  if (result < 0) {
    if (format_context != nullptr) {
      avformat_free_context(format_context);
    }
    throw ascii::ffmpeg_error("Failed to find stream info for " + file_path, result);
  }

  if (format_context != nullptr) {
    return format_context;
  }
  throw std::runtime_error("Failed to open format context input, unknown error occured");
}

void dump_file_info(const char* file_path) {
  av_log_set_level(AV_LOG_INFO);
  AVFormatContext* format_context = open_format_context(file_path);
  av_dump_format(format_context, 0, file_path, 0);
  avformat_free_context(format_context);
}

double get_file_duration(const char* file_path) {
  AVFormatContext* format_context = open_format_context(file_path);
  int64_t duration = format_context->duration;
  double duration_seconds = (double)duration / AV_TIME_BASE;
  avformat_free_context(format_context);
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

bool avformat_context_is_static_image(AVFormatContext* format_context) {
  return avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_VIDEO) &&
  (format_context->duration == AV_NOPTS_VALUE || format_context->duration == 0) &&
  (format_context->start_time == AV_NOPTS_VALUE || format_context->start_time == 0);
}

bool avformat_context_is_video(AVFormatContext* format_context) {
  return !avformat_context_is_static_image(format_context) &&
  avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_VIDEO);
}

bool avformat_context_is_audio_only(AVFormatContext* format_context) {
  return !avformat_context_is_static_image(format_context) &&
  !avformat_context_is_video(format_context) &&
  avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_AUDIO);
}