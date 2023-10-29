#ifndef ASCII_VIDEO_ASCII_VIDEO_H
#define ASCII_VIDEO_ASCII_VIDEO_H

#include "playlist.h"
#include "pixeldata.h"

#include <optional>
#include <vector>
#include <string>

extern const std::string ASCII_VIDEO_CONTROLS_USAGE;

enum class VideoOutputMode {
  COLORED,
  GRAYSCALE,
  COLORED_BACKGROUND_ONLY,
  GRAYSCALE_BACKGROUND_ONLY,
  TEXT_ONLY,
};

struct AsciiVideoProgramData {
  Playlist playlist;
  double volume;
  bool muted;
  VideoOutputMode vom;
  ScalingAlgo scaling_algorithm;
  std::optional<int> render_loop_max_fps;
  bool fullscreen;
  std::string ascii_display_chars;
};

int ascii_video(AsciiVideoProgramData avpd);

#endif