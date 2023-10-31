#ifndef TMEDIA_MEDIA_TYPE_H
#define TMEDIA_MEDIA_TYPE_H

#include <string>

extern "C" {
  #include <libavformat/avformat.h>
}

enum class MediaType {
  VIDEO,
  AUDIO,
  IMAGE
};

std::string media_type_to_string(MediaType media_type);
MediaType media_type_from_avformat_context(AVFormatContext* format_context);

#endif