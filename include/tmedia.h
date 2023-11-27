#ifndef TMEDIA_TMEDIA_H
#define TMEDIA_TMEDIA_H

#include "playlist.h"
#include "pixeldata.h"
#include "tmedia_vom.h"
#include "metadata.h"
#include "boiler.h"
#include "scale.h"

#include "audioout.h"
#include "mediafetcher.h"

#include <optional>
#include <vector>
#include <string>

extern const std::string TMEDIA_CONTROLS_USAGE;

struct TMediaStartupState {
  Playlist playlist;
  double volume;
  bool muted;
  int refresh_rate_fps;
  
  VideoOutputMode vom;
  ScalingAlgo scaling_algorithm;
  bool fullscreen;
  std::string ascii_display_chars;
};

TMediaStartupState tmedia_parse_cli(int argc, char** argv);

enum class TMediaCommand {
  // playlist commands
  SKIP,
  REWIND,
  SHUFFLE,
  UNSHUFFLE,
  TOGGLE_SHUFFLE,
  SET_LOOP_TYPE,

  // Playback Commands
  SEEK,
  SEEK_OFFSET,
  PLAY,
  PAUSE,
  TOGGLE_PLAYBACK,

  // Visual commands
  SET_VIDEO_OUTPUT_MODE,

  // volume controls
  SET_VOLUME,
  VOLUME_OFFSET,
  MUTE
};

struct TMediaCommandData {
  TMediaCommand name;
  std::string payload;
};

struct TMediaProgramState {
  Playlist playlist;
  double volume;
  bool muted;
  bool quit;
  bool fullscreen;
  int refresh_rate_fps;
  bool playing;
  ScalingAlgo scaling_algorithm;
  VideoOutputMode vom;
  std::string ascii_display_chars;
};

struct TMediaProgramSnapshot {
  PixelData frame;
  MediaType media_type;
  double media_duration_secs;
  double media_time_secs;
  bool has_audio_output;
};


class TMediaRenderer {
  private:
    MetadataCache metadata_cache;
    VideoDimensions last_frame_dims;
    void render_tui_fullscreen(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot);
    void render_tui_compact(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot);
    void render_tui_large(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot);
  
  public:
    void render_tui(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot);
};

void tmedia_handle_key(int key);


int tmedia(TMediaStartupState tmpd);

#endif