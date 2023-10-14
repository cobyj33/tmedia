#include "mediatype.h"

#include "boiler.h"

#include <stdexcept>

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
  if (avformat_context_is_static_image(format_context)) {
    return MediaType::IMAGE;
  } else if (avformat_context_is_video(format_context)) {
    return MediaType::VIDEO;
  } else if (avformat_context_is_audio_only(format_context)) {
    return MediaType::AUDIO;
  } else {
    throw std::runtime_error("Could not find matching MediaType for AVFormatContext for file " + std::string(format_context->url));
  }
}