
#include "boiler.h"

#include "decode.h"
#include "ffmpeg_error.h"
#include "formatting.h"
#include "avguard.h"

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

const char* banned_iformat_names = "tty";

const char* image_iformat_names = "image2,png_pipe,webp_pipe";
const char* audio_iformat_names = "wav,ogg,mp3,flac";
const char* video_iformat_names = "flv";

const char* image_iformat_exts= "jpg,jpeg,png,webp";
const char* audio_iformat_exts= "mp3,flac,wav,ogg";
const char* video_iformat_exts= "flv";

bool av_match_lists(const char* first, char fsep, const char* sec, char ssep) {
  std::vector<std::string> to_check = strsplit(first, fsep);

  for (const std::string& str : to_check) {
    if (av_match_list(str.c_str(), sec, ssep)) return true;
  }

  return false;
}

AVFormatContext* open_format_context(const std::string& file_path) {
  AVFormatContext* format_context = nullptr;
  int result = avformat_open_input(&format_context, file_path.c_str(), nullptr, nullptr);
  if (result < 0) {
    if (format_context != nullptr) {
      avformat_close_input(&format_context);
    }
    throw ffmpeg_error("[open_format_context] Failed to open format context input for " + file_path, result);
  }

  if (av_match_lists(format_context->iformat->name, ',', banned_iformat_names, ',')) {
    throw std::runtime_error("[open_format_context] Cannot open banned format type: " + std::string(format_context->iformat->name));
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
  #if AVFORMAT_CONST_AVIOFORMAT
  const AVInputFormat* avif;
  #else
  AVInputFormat* avif;
  #endif
  int score = 0;
};

AVProbeFileRet av_probe_file(const std::string& path_str) {
  AVProbeFileRet res;

  AVIOContext* avio_ctx;
  int ret = avio_open(&avio_ctx, path_str.c_str(), AVIO_FLAG_READ);
  if (ret < 0) {
    throw ffmpeg_error("[av_probe_file] avio_open failure", ret);
  }

  #if AVFORMAT_CONST_AVIOFORMAT
  const AVInputFormat* avif = NULL;
  #else
  AVInputFormat* avif = NULL;
  #endif

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

std::optional<MediaType> media_type_from_mime_type(std::string_view mime_type) {
  std::string_view gen_type = str_until(mime_type, '/'); 

  if (gen_type == "image") { 
    return MediaType::IMAGE;
  } else if (gen_type == "audio") {
    return MediaType::AUDIO;
  } else if (gen_type == "video") {
    return MediaType::VIDEO;
  }
  return std::nullopt;
}

std::optional<MediaType> media_type_from_iformat(const AVInputFormat* iformat) {
  if (av_match_lists(iformat->name, ',', banned_iformat_names, ',')) {
    return std::nullopt;
  }

  if (av_match_lists(iformat->name, ',', image_iformat_names, ',')) {
    return MediaType::IMAGE;
  }

  if (av_match_lists(iformat->name, ',', audio_iformat_names, ',')) {
    return MediaType::AUDIO;
  }

  if (av_match_lists(iformat->name, ',', video_iformat_names, ',')) {
    return MediaType::VIDEO;
  }

  if (iformat->mime_type != nullptr) {
    std::vector<std::string> mime_types = strsplit(iformat->mime_type, ',');
    std::optional<MediaType> resolved_media_type = std::nullopt;

    for (const std::string& str : mime_types) {
      std::optional<MediaType> current_media_type = media_type_from_mime_type(str);
      if (current_media_type.has_value() && resolved_media_type.has_value() && resolved_media_type != current_media_type) {
        resolved_media_type = std::nullopt;
        break;
      } else {
        resolved_media_type = current_media_type;
      }
    }

    if (resolved_media_type != std::nullopt) return resolved_media_type;
  }

  return std::nullopt;
}

bool probably_valid_media_file_path(const std::string& path_str) {
  std::string filename = to_filename(path_str);
  const AVOutputFormat* fmt = av_guess_format(NULL, filename.c_str(), NULL);
  if (fmt != NULL) return true;
  
  try {
    AVProbeFileRet pfret = av_probe_file(path_str);
    if (pfret.score > AVPROBE_SCORE_RETRY) return true;
  } catch (const std::runtime_error& e) {
    // no-op
  }

  return is_valid_media_file_path(path_str);
}

std::optional<MediaType> media_type_guess(const std::string& path_str) {
  try {
    AVProbeFileRet pfret = av_probe_file(path_str);
    if (pfret.score > AVPROBE_SCORE_RETRY) {
      if (std::optional<MediaType> from_iformat = media_type_from_iformat(pfret.avif)) {
        return from_iformat;
      }
    }
  } catch (const std::runtime_error& e) {
    // no-op
  }

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
    bool valid = true;
    AVFormatContext* fmt_ctx = open_format_context(path_str);
    valid = valid && !av_match_list(fmt_ctx->iformat->name, banned_iformat_names, ',');
    avformat_close_input(&fmt_ctx);
    return valid;
  } catch (const std::runtime_error& e) {
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
  if (std::optional<MediaType> from_iformat = media_type_from_iformat(format_context->iformat)) {
    return from_iformat.value();
  }

  if (avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_VIDEO)) {
    if (!avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_AUDIO) &&
    (format_context->duration == AV_NOPTS_VALUE || format_context->duration == 0) &&
    (format_context->start_time == AV_NOPTS_VALUE || format_context->start_time == 0)) {
      return MediaType::IMAGE;
    }

    bool video_is_attached_pic = true;
    for (std::size_t i = 0; i < format_context->nb_streams; i++) {
      if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        video_is_attached_pic = video_is_attached_pic && format_context->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC;
      }
    }

    if (video_is_attached_pic) {
      return avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_AUDIO) ? MediaType::AUDIO : MediaType::IMAGE;
    }
    return MediaType::VIDEO;
  }
  
  if (avformat_context_has_media_stream(format_context, AVMEDIA_TYPE_AUDIO)) {
    return MediaType::AUDIO;
  }

  throw std::runtime_error("[media_type_from_avformat_context] Could not find media type for file " + std::string(format_context->url));
}
