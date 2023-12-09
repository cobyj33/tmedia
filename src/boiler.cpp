
#include "boiler.h"

#include "decode.h"
#include "ffmpeg_error.h"
#include "formatting.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <string_view>

#include <filesystem>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libavutil/dict.h>
#include <libavutil/avstring.h>
}

const char* image_iformat_names = "image2,png_pipe,webp_pipe";
const char* audio_iformat_names = "wav,ogg,mp3,flac";
const char* video_iformat_names = "flv";
const char* banned_iformat_names = "tty";

AVFormatContext* open_format_context(const std::string& file_path) {
  AVFormatContext* format_context = nullptr;
  int result = avformat_open_input(&format_context, file_path.c_str(), nullptr, nullptr);
  if (result < 0) {
    if (format_context != nullptr) {
      avformat_close_input(&format_context);
    }
    throw ffmpeg_error("[open_format_context] Failed to open format context input for " + file_path, result);
  }

  result = avformat_find_stream_info(format_context, NULL);
  if (result < 0) {
    if (format_context != nullptr) {
      avformat_close_input(&format_context);
    }
    throw ffmpeg_error("[open_format_context] Failed to find stream info for " + file_path, result);
  }

  if (format_context != nullptr) {
    return format_context;
  }
  throw std::runtime_error("[open_format_context] Failed to open format context input, unknown error occured");
}

void dump_file_info(const std::string& file_path) {
  AVFormatContext* format_context = open_format_context(file_path);
  dump_format_context(format_context);
  avformat_close_input(&format_context);
}

void dump_format_context(AVFormatContext* format_context) {
  const int saved_avlog_level = av_log_get_level();
  av_log_set_level(AV_LOG_INFO);
  av_dump_format(format_context, 0, format_context->url, 0);
  av_log_set_level(saved_avlog_level);
}

double get_file_duration(const std::string& file_path) {
  AVFormatContext* format_context = open_format_context(file_path);
  int64_t duration = format_context->duration;
  double duration_seconds = (double)duration / AV_TIME_BASE;
  avformat_close_input(&format_context);
  return duration_seconds;
}

struct AVProbeFileRet {
  AVInputFormat* avif;
  int score = 0;
};

AVProbeFileRet av_probe_file(const std::string& path_str) {
  AVProbeFileRet res;

  AVIOContext* avio_ctx;
  int ret = avio_open(&avio_ctx, path_str.c_str(), AVIO_FLAG_READ);
  if (ret < 0) {
    throw ffmpeg_error("[av_probe_file] avio_open failure", ret);
  }

  AVInputFormat* avif;
  res.score = av_probe_input_buffer2(avio_ctx, &avif, path_str.c_str(), NULL, 0, 0);
  avio_close(avio_ctx);
  res.avif = avif;
 
  if (res.score < 0) {
    throw ffmpeg_error("[av_probe_file] av_probe_input_buffer2 failure", res.score);
  }

  return res;
}


bool avformat_context_has_media_stream(AVFormatContext* format_context, enum AVMediaType media_type) {
  for (unsigned int i = 0; i < format_context->nb_streams; i++) {
    if (format_context->streams[i]->codecpar->codec_type == media_type) {
      return true;
    }
  }
  return false;
}

// MediaType media_type_from_avinput_format(AVInputFormat* input_fmt) {
//   input_fmt->
  
// }

bool probably_valid_media_file_path(const std::string& path_str) {
  std::string filename = to_filename(path_str);
  const AVOutputFormat* fmt = av_guess_format(NULL, filename.c_str(), NULL);
  if (fmt != NULL) return true;
  // AVProbeFileRet pfret = av_probe_file(path_str);
  // if (pfret.score > AVPROBE_SCORE_RETRY) return true;
  return is_valid_media_file_path(path_str);
}

std::optional<MediaType> media_type_guess(const std::string& path_str) {
  std::string ext = to_file_ext(path_str);

  static std::pair<std::string_view, MediaType> extensions[] = {
    {"flv", MediaType::VIDEO},
    {"mp3", MediaType::AUDIO},
    {"flac", MediaType::AUDIO},
    {"wav", MediaType::AUDIO},
    {"ogg", MediaType::AUDIO},
    {"jpg", MediaType::IMAGE},
    {"jpeg", MediaType::IMAGE},
    {"png", MediaType::IMAGE},
    {"webp", MediaType::IMAGE},
  };

  for (std::pair<std::string_view, MediaType>& pair : extensions) {
    if (pair.first == ext) return pair.second;
  }

  // AVProbeFileRet pfret = av_probe_file(path_str);
  // if (pfret.score > AVPROBE_SCORE_RETRY) {
  // }

  try {
    AVFormatContext* fmt_ctx = open_format_context(path_str);
    MediaType media_type = media_type_from_avformat_context(fmt_ctx);
    avformat_close_input(&fmt_ctx);
    return media_type;
  } catch (const std::runtime_error& e) {
    // no-op
  }

  return std::nullopt;
}

bool is_valid_media_file_path(const std::string& path_str) {
  std::filesystem::path path(path_str);
  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec)) return false;

  try {
    AVFormatContext* fmt_ctx = open_format_context(path_str);

    if (av_match_list(fmt_ctx->iformat->name, banned_iformat_names, ',')) {
      avformat_close_input(&fmt_ctx);
      return false;
    }

    avformat_close_input(&fmt_ctx);
    return true;
  } catch (const ffmpeg_error& e) {
    return false;
  }

  return false;
}

std::string media_type_to_string(MediaType media_type) {
  switch (media_type) {
    case MediaType::VIDEO: return "video";
    case MediaType::AUDIO: return "audio";
    case MediaType::IMAGE: return "image";
  }
  throw std::runtime_error("[media_type_to_string] attempted to get media type string from unimplemented media type.");
}

/**
 * Note that for media_type_from_avformat_context, the recognized iformat names
 * can only be used to correctly identify media file formats that have one specific
 * file type. Things like mp4 and webm are more so containers and really can't be
 * guaranteed to be identifiable from a format name.
 * 
 * Also, note that the iformat->name can be a comma separated list of names for
 * different media file types. This isn't implemented here, as the media types
 * here should all just be the given name, but it would be of note if trying
 * to implement new file formats.
*/

MediaType media_type_from_avformat_context(AVFormatContext* format_context) {
  if (av_match_list(format_context->iformat->name, banned_iformat_names, ',')) {
    throw std::runtime_error("[media_type_from_avformat_context] Could not find media type for banned media type: " + std::string(format_context->iformat->name));
  }

  if (av_match_list(format_context->iformat->name, image_iformat_names, ',')) {
    return MediaType::IMAGE;
  } 

  if (av_match_list(format_context->iformat->name, audio_iformat_names, ',')) {
    return MediaType::AUDIO;
  }

  if (av_match_list(format_context->iformat->name, video_iformat_names, ',')) {
    return MediaType::VIDEO;
  }

  if (avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_VIDEO)) {
    if (!avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_AUDIO) &&
    (format_context->duration == AV_NOPTS_VALUE || format_context->duration == 0) &&
    (format_context->start_time == AV_NOPTS_VALUE || format_context->start_time == 0)) {
      return MediaType::IMAGE;
    }

    return MediaType::VIDEO;
  } else if (avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_AUDIO)) {
    return MediaType::AUDIO;
  }

  throw std::runtime_error("[media_type_from_avformat_context] Could not find media type for file " + std::string(format_context->url));
}
