#ifndef TMEDIA_TMEDIA_FILE_SEARCH_H
#define TMEDIA_TMEDIA_FILE_SEARCH_H

#include <vector>
#include <filesystem>
#include <optional>


struct SearchPathOptions {
  std::optional<bool> ignore_images;
  std::optional<bool> ignore_audio;
  std::optional<bool> ignore_video;
  std::optional<bool> recurse;
};

struct SearchPath {
  std::filesystem::path path;
  SearchPathOptions options;
};

class DirectorySearch {
  std::vector<std::filesystem::path> omitted;
  std::vector<SearchPath> paths_to_search;
};

#endif