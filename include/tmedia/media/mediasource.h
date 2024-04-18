#ifndef TMEDIA_MEDIA_SOURCE_H
#define TMEDIA_MEDIA_SOURCE_H

#include <filesystem>
#include <optional>

#include <tmedia/media/metadata.h>

struct MediaSourceOptions {
  std::optional<double> image_display_time_secs; // slideshow definition
};

// currently unused, as I'm not sure what data I'd like to use for each entry
class MediaSource {
  std::filesystem::path path;
  Metadata metadata;
  MediaSourceOptions opts;
  
  MediaSource() {}
  MediaSource(const std::filesystem::path& path, int i) : path(path) {} 
};

#endif