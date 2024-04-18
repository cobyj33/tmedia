#ifndef TMEDIA_MEDIA_SOURCE_H
#define TMEDIA_MEDIA_SOURCE_H

#include <filesystem>

#include <tmedia/media/metadata.h>

// currently unused, as I'm not sure what data I'd like to use for each entry
class MediaSource {
  std::filesystem::path path;
  Metadata metadata;
  
  MediaSource() {}
  MediaSource(const std::filesystem::path& path, int i) : path(path) {} 
};

#endif