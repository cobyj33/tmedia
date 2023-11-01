#ifndef TMEDIA_TMEDIA_H
#define TMEDIA_TMEDIA_H

#include "playlist.h"
#include "pixeldata.h"
#include "tmedia_vom.h"

#include <optional>
#include <vector>
#include <string>

extern const std::string TMEDIA_CONTROLS_USAGE;

struct TMediaProgramData {
  Playlist playlist;
  double volume;
  bool muted;
  VideoOutputMode vom;
  ScalingAlgo scaling_algorithm;
  std::optional<int> render_loop_max_fps;
  bool fullscreen;
  std::string ascii_display_chars;
};

int tmedia(TMediaProgramData tmpd);

#endif