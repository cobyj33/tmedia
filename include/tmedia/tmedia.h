#ifndef TMEDIA_TMEDIA_H
#define TMEDIA_TMEDIA_H


#include <tmedia/image/ascii.h> // for VidOutMode
#include <tmedia/tmedia_vom.h> // for VidOutMode
#include <tmedia/media/playlist.h> // for LoopType 
#include <tmedia/media/metadata.h> // for MetadataCache
#include <tmedia/image/scale.h> // for Dim2
#include <tmedia/ffmpeg/boiler.h> // for MediaType
#include <tmedia/image/pixeldata.h> // for PixelData and ScalingAlgo
#include <tmedia/util/funcmac.h> // for ASCII_STANDARD_CHAR_MAP

#include <optional>
#include <vector>
#include <string>
#include <filesystem>

extern const char* TMEDIA_CONTROLS_USAGE;
extern const char* TMEDIA_CLI_OPTIONS_DESC;

struct TMediaStartupState {
  std::vector<std::filesystem::path> media_files;
  LoopType loop_type = LoopType::NO_LOOP;
  bool shuffled = false;
  double volume = 1.0;
  bool muted = false;
  int refresh_rate_fps = 24;
  VidOutMode vom = VidOutMode::PLAIN;
  ScalingAlgo scaling_algorithm = ScalingAlgo::BOX_SAMPLING;
  bool fullscreen = false;
  std::string ascii_display_chars = ASCII_STANDARD_CHAR_MAP;
};

int tmedia_run(TMediaStartupState& tmpd);


struct TMediaCLIParseRes {
  TMediaStartupState tmss;
  bool exited;
  TMediaCLIParseRes(const TMediaStartupState& tmss, bool exited) : tmss(tmss), exited(exited) {}
  TMediaCLIParseRes(TMediaStartupState&& tmss, bool exited) : tmss(std::move(tmss)), exited(exited) {}
};

//currently unimplemented
TMediaCLIParseRes tmedia_parse_cli(int argc, char** argv);


struct TMediaProgramState {
  Playlist plist;
  double volume = 1.0;
  bool muted = false;
  bool quit = false;
  bool fullscreen = false;
  int refresh_rate_fps = 24;
  ScalingAlgo scaling_algorithm = ScalingAlgo::BOX_SAMPLING;
  VidOutMode vom = VidOutMode::PLAIN;
  std::string ascii_display_chars = ASCII_STANDARD_CHAR_MAP;
  Dim2 req_frame_dim = Dim2(1, 1);
};

struct TMediaProgramSnapshot {
  std::string currently_playing;
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
    virtual std::vector<TMediaCommandData> process_input(const TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot) = 0;
};

class TMediaRenderer {
  public:
    virtual void render(TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot) = 0;
};

class TMediaCursesRenderer : public TMediaRenderer {
  private:
    MetadataCache metadata_cache;
    Dim2 last_frame_dims;
    void render_tui_fullscreen(TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot);
    void render_tui_compact(TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot);
    void render_tui_large(TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot);
  
  public:
    void render(TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot);
};

class TMediaCursesCommandHandler : public TMediaCommandHandler {
  public:
    std::vector<TMediaCommandData> process_input(const TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot);
};


#endif