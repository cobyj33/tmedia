#ifndef TMEDIA_TMEDIA_H
#define TMEDIA_TMEDIA_H

#include "playlist.h"
#include "pixeldata.h"

#include <optional>
#include <vector>
#include <string>

extern const std::string TMEDIA_CONTROLS_USAGE;

enum class VideoOutputMode {
  COLORED,
  GRAYSCALE,
  COLORED_BACKGROUND_ONLY,
  GRAYSCALE_BACKGROUND_ONLY,
  TEXT_ONLY,
};

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