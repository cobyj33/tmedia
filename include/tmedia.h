#ifndef TMEDIA_TMEDIA_H
#define TMEDIA_TMEDIA_H

#include "playlist.h"
#include "pixeldata.h"
#include "tmedia_vom.h"
#include "metadata.h"
#include "boiler.h"
#include "scale.h"
#include "ascii.h"

#include "audioout.h"
#include "mediafetcher.h"

#include <optional>
#include <vector>
#include <string>
#include <filesystem>

extern const std::string TMEDIA_CONTROLS_USAGE;


struct TMediaStartupState {
  std::vector<std::filesystem::path> media_files;
  LoopType loop_type = LoopType::NO_LOOP;
  bool shuffled = false;
  double volume = 1.0;
  bool muted = false;
  int refresh_rate_fps = 24;
  VideoOutputMode vom = VideoOutputMode::TEXT_ONLY;
  ScalingAlgo scaling_algorithm = ScalingAlgo::BOX_SAMPLING;
  bool fullscreen = false;
  std::string ascii_display_chars = ASCII_STANDARD_CHAR_MAP;
};

int tmedia_run(TMediaStartupState tmpd);


struct TMediaCLIParseRes {
  TMediaStartupState tmss;
  bool exited;
  TMediaCLIParseRes(TMediaStartupState tmss, bool exited) : tmss(tmss), exited(exited) {}
};

//currently unimplemented
TMediaCLIParseRes tmedia_parse_cli(int argc, char** argv);

struct TMediaProgramState {
  Playlist<std::filesystem::path> playlist;
  double volume = 1.0;
  bool muted = false;
  bool quit = false;
  bool fullscreen = false;
  int refresh_rate_fps = 24;
  ScalingAlgo scaling_algorithm = ScalingAlgo::BOX_SAMPLING;
  VideoOutputMode vom = VideoOutputMode::TEXT_ONLY;
  std::string ascii_display_chars = ASCII_STANDARD_CHAR_MAP;
};

struct TMediaProgramSnapshot {
  PixelData frame;
  MediaType media_type;
  bool playing;
  double media_duration_secs;
  double media_time_secs;
  bool has_audio_output;
};

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
  RESIZE,
  REFRESH,
  FULLSCREEN,
  NO_FULLSCREEN,
  TOGGLE_FULLSCREEN,

  // volume controls
  SET_VOLUME,
  VOLUME_OFFSET,
  MUTE
};

struct TMediaCommandData {
  TMediaCommand name;
  std::string payload; // serialized string of sent data
};

class TMediaCommandHandler {
  public:
    virtual std::vector<TMediaCommandData> process_input(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot) = 0;
};

class TMediaRenderer {
  public:
    virtual void render(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot) = 0;
};

class TMediaCursesRenderer : public TMediaRenderer {
  private:
    MetadataCache metadata_cache;
    VideoDimensions last_frame_dims;
    void render_tui_fullscreen(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot);
    void render_tui_compact(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot);
    void render_tui_large(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot);
  
  public:
    void render(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot);
};

class TMediaCursesCommandHandler : public TMediaCommandHandler {
  public:
    std::vector<TMediaCommandData> process_input(const TMediaProgramState tmps, const TMediaProgramSnapshot snapshot);
};


#endif