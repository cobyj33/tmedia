#ifndef TMEDIA_TMEDIA_H
#define TMEDIA_TMEDIA_H

#include "playlist.h"
#include "pixeldata.h"
#include "tmedia_vom.h"
#include "metadata.h"
#include "boiler.h"
#include "scale.h"


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

struct TMediaProgramState {
  Playlist playlist;
  double volume;
  bool muted;

  bool quit;

  bool is_playing;
  double media_duration_secs;
  double media_time_secs;
  MediaType media_type;

  bool has_audio_output;

  int refresh_rate_fps;

  PixelData frame;
  VideoOutputMode vom;
  bool fullscreen;
  ScalingAlgo scaling_algorithm;
  std::string ascii_display_chars;
};


class TMediaRenderer {
  private:
    MetadataCache metadata_cache;
    VideoDimensions last_frame_dims;
    void render_tui_fullscreen(const TMediaProgramState tmps);
    void render_tui_compact(const TMediaProgramState tmps);
    void render_tui_large(const TMediaProgramState tmps);
  
  public:
    void render_tui(TMediaProgramState tmps);
};


// struct ExitAction {};
// struct IncrementVolumeAction { double increment; };
// struct SeekAction { double time; };
// struct SeekRelativeAction { double offset; };
// struct ToggleMuteAction {};
// struct PlayAction {};
// struct PauseAction {};
// struct TogglePlayAction {};
// struct SkipAction {};
// struct RewindAction {};
// struct GrayscaleAction {};
// struct BackgroundAction {};
// struct ColorAction {};
// struct LoopAction {};

void tmedia_handle_key(int key);


int tmedia(TMediaStartupState tmpd);

#endif