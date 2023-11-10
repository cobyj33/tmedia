#include "mediatype.h"

#include "boiler.h"

#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
  #include <libavformat/avformat.h>
  #include <libavutil/avstring.h>
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
  const char* image_iformat_names = "image2,png_pipe,webp_pipe";
  const char* audio_iformat_names = "wav,ogg,mp3,flac";
  const char* video_iformat_names = "flv";

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