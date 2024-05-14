#ifndef MEDIATYPE_H
#define MEDIATYPE_H

#include <optional>
#include <string_view>

enum class MediaType {
  VIDEO,
  AUDIO,
  IMAGE
};

constexpr const char* media_type_cstr(MediaType media_type);
std::optional<MediaType> media_type_from_mime_type(std::string_view mime_type);

#endif