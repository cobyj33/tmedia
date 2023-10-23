#ifndef ASCII_VIDEO_MEDIA_PLAYER_H
#define ASCII_VIDEO_MEDIA_PLAYER_H

#include "looptype.h"

#include <vector>
#include <string>
#include <cstddef>

class MediaPlayer {
  std::vector<std::string> files;
  std::size_t current_file;
  LoopType loop_type;

  MediaPlayer(const std::vector<std::string>& files);

  void run();
}


#endif