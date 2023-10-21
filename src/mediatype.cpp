#include "mediatype.h"

#include "boiler.h"

#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
  #include <libavformat/avformat.h>
}

std::string media_type_to_string(MediaType media_type) {
  switch (media_type) {
    case MediaType::VIDEO: return "video";
    case MediaType::AUDIO: return "audio";
    case MediaType::IMAGE: return "image";
  }
  throw std::runtime_error("Attempted to get Media Type string from unimplemented Media Type. Please implement all MediaType's to return a valid string. This is a developer error and bug.");
}

MediaType media_type_from_avformat_context(AVFormatContext* format_context) {
  std::vector<std::string> image_formats{"image2", "png_pipe", "webp_pipe"};
  std::vector<std::string> audio_formats{"wav", "ogg", "mp3", "flac"};
  for (std::size_t i = 0; i < image_formats.size(); i++) {
    if (strcmp(format_context->iformat->name, image_formats[i].c_str()) == 0) {
      return MediaType::IMAGE;
    }
  }

  for (std::size_t i = 0; i < audio_formats.size(); i++) {
    if (strcmp(format_context->iformat->name, audio_formats[i].c_str()) == 0) {
      return MediaType::AUDIO;
    }
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