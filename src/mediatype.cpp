#include "mediatype.h"

#include "boiler.h"

#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include <string_view>

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
  static const std::set<std::string_view> image_iformat_names{"image2", "png_pipe", "webp_pipe"};
  static const std::set<std::string_view> audio_iformat_names{"wav", "ogg", "mp3", "flac"};
  static const std::set<std::string_view> video_iformat_names{"flv"};

  if (image_iformat_names.count(format_context->iformat->name) == 1) {
    return MediaType::IMAGE;
  } 

  if (audio_iformat_names.count(format_context->iformat->name) == 1) {
    return MediaType::AUDIO;
  }

  if (video_iformat_names.count(format_context->iformat->name) == 1) {
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