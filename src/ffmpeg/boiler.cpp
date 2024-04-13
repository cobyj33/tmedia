
#include <tmedia/ffmpeg/boiler.h>

#include <tmedia/ffmpeg/decode.h>
#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/util/formatting.h>
#include <tmedia/ffmpeg/avguard.h>
#include <tmedia/media/mediaformat.h>
#include <tmedia/util/funcmac.h>

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <string_view>

#include <filesystem>

#include <fmt/format.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libavutil/dict.h>
#include <libavutil/avstring.h>
}

AVFormatContext* open_format_context(const std::filesystem::path& path) {
  AVFormatContext* fmt_ctx = nullptr;
  int result = avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr);
  if (result < 0) {
    throw ffmpeg_error(fmt::format("[{}] Failed to open format context input "
    "for {}", FUNCDINFO, path.string()), result);
  }

  if (av_match_list(fmt_ctx->iformat->name, banned_iformat_names, ',')) {
    std::string iformatname(fmt_ctx->iformat->name);
    avformat_close_input(&fmt_ctx);
    throw std::runtime_error(fmt::format("[{}] Cannot open banned format type: "
    "{}", FUNCDINFO, iformatname));
  }

  result = avformat_find_stream_info(fmt_ctx, NULL);
  if (result < 0) {
    avformat_close_input(&fmt_ctx);
    throw ffmpeg_error(fmt::format("[{}] Failed to find stream info for {}.",
    FUNCDINFO, path.string()), result);
  }

  if (fmt_ctx != nullptr) {
    return fmt_ctx;
  }

  throw std::runtime_error(fmt::format("[{}] Failed to open format context "
  "input, unknown error occured", FUNCDINFO));
}

void dump_file_info(const std::filesystem::path& path) {
  AVFormatContext* fmt_ctx = open_format_context(path);
  dump_format_context(fmt_ctx);
  avformat_close_input(&fmt_ctx);
}

void dump_format_context(AVFormatContext* fmt_ctx) {
  const int saved_avlog_level = av_log_get_level();
  av_log_set_level(AV_LOG_INFO);
  av_dump_format(fmt_ctx, 0, fmt_ctx->url, 0);
  av_log_set_level(saved_avlog_level);
}

double get_file_duration(const std::filesystem::path& path) {
  AVFormatContext* fmt_ctx = open_format_context(path);
  int64_t duration = fmt_ctx->duration;
  double duration_seconds = (double)duration / AV_TIME_BASE;
  avformat_close_input(&fmt_ctx);
  return duration_seconds;
}


bool avformat_context_has_media_stream(AVFormatContext* fmt_ctx, enum AVMediaType media_type) {
  for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == media_type) {
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

const char* media_type_cstr(MediaType media_type) {
  switch (media_type) {
    case MediaType::VIDEO: return "video";
    case MediaType::AUDIO: return "audio";
    case MediaType::IMAGE: return "image";
  }
  return "unknown"; // unreachable
}

std::optional<MediaType> media_type_from_iformat(const AVInputFormat* iformat) {
  if (av_match_list(iformat->name, banned_iformat_names, ',')) {
    return std::nullopt;
  }

  if (av_match_list(iformat->name, image_iformat_names, ',')) {
    return MediaType::IMAGE;
  }

  if (av_match_list(iformat->name, audio_iformat_names, ',')) {
    return MediaType::AUDIO;
  }

  if (av_match_list(iformat->name, video_iformat_names, ',')) {
    return MediaType::VIDEO;
  }

  if (iformat->mime_type != nullptr) {
    std::vector<std::string_view> mime_types = strvsplit(iformat->mime_type, ',');
    std::optional<MediaType> resolved_media_type = std::nullopt;

    for (const std::string_view str : mime_types) {
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

MediaType media_type_from_avformat_context(AVFormatContext* fmt_ctx) {
  if (std::optional<MediaType> from_iformat = media_type_from_iformat(fmt_ctx->iformat)) {
    return from_iformat.value();
  }

  if (avformat_context_has_media_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO)) {
    if (!avformat_context_has_media_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO) &&
    (fmt_ctx->duration == AV_NOPTS_VALUE || fmt_ctx->duration == 0) &&
    (fmt_ctx->start_time == AV_NOPTS_VALUE || fmt_ctx->start_time == 0)) {
      return MediaType::IMAGE;
    }

    bool video_is_attached_pic = true;
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
      if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        video_is_attached_pic = video_is_attached_pic && fmt_ctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC;
      }
    }

    if (video_is_attached_pic) {
      return avformat_context_has_media_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO) ? MediaType::AUDIO : MediaType::IMAGE;
    }
    
    return MediaType::VIDEO;
  }
  
  if (avformat_context_has_media_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO)) {
    return MediaType::AUDIO;
  }

  throw std::runtime_error(fmt::format("[{}] Could not find media type for "
  "file {}.", FUNCDINFO, std::string(fmt_ctx->url)));
}
