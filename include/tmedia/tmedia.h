#ifndef TMEDIA_TMEDIA_H
#define TMEDIA_TMEDIA_H


#include <tmedia/image/ascii.h> // for ASCII_STANDARD_CHAR_MAP
#include <tmedia/media/playlist.h> // for LoopType
#include <tmedia/media/metadata.h> // for MetadataCache
#include <tmedia/image/scale.h> // for Dim2
#include <tmedia/ffmpeg/boiler.h> // for MediaType
#include <tmedia/image/pixeldata.h> // for PixelData

#include <optional>
#include <vector>
#include <string>
#include <filesystem>

extern const char* TMEDIA_CONTROLS_USAGE;
extern const char* TMEDIA_CLI_ARGS_DESC;

enum class VidOutMode {
  PLAIN,
  COLOR,
  GRAY,
  COLOR_BG,
  GRAY_BG
};


struct TMediaStartupState {
  std::vector<PlaylistItem> media_files;
  LoopType loop_type = LoopType::NO_LOOP;
  bool shuffled = false;
  double volume = 1.0;
  bool muted = false;
  int refresh_rate_fps = 24;
  VidOutMode vom = VidOutMode::PLAIN;
  bool fullscreen = false;
  std::string ascii_display_chars = ASCII_STANDARD_CHAR_MAP;
};

int tmedia_run(TMediaStartupState& tmpd);


struct TMediaCLIParseRes {
  TMediaStartupState tmss;
  bool exited;
  std::vector<std::string> errors;
};

TMediaCLIParseRes tmedia_parse_cli(int argc, char** argv);

/**
 * 
 * Note that this isn't all of the state of tmedia, but just configurable
 * state by the user when invoking tmedia from the command line. However,
 * calling it "Config" state isn't really correct since these values change
 * throughout the running of tmedia.
 */
struct TMediaProgramState {
  Playlist plist;
  double volume = 1.0;
  bool muted = false;
  bool fullscreen = false;
  bool show_help_design = false;
  int refresh_rate_fps = 24;
  VidOutMode vom = VidOutMode::PLAIN;
  std::string ascii_display_chars = ASCII_STANDARD_CHAR_MAP;
};

/**
 * A snapshot of information about the current state of tmedia. This struct
 * exists to pass along data to rendering functions without exposing the actual
 * ability to control tmedia's state toward UI rendering. Therefore, if anything
 * in tmedia's state begins acting weird, we know it is not a fault of any
 * rendering data.
 */
struct TMediaProgramSnapshot {
  PixelData* frame;
  MediaType media_type;
  bool playing;
  double media_duration_secs;
  double media_time_secs;
  bool has_audio_output;
  bool should_render_frame;
};

/**
 * State necessary to hold between calls to rendering tmedia's tui. This is what
 * many like to call a "leaky abstraction", but it works and it works well,
 * therefore I don't care.
*/
struct TMediaRendererState {
  /**
   * A cache of metadata values for media files. Since fetching metadata is
   * an expensive operation that includes looking into the filesystem and
   * opening an AVFormatContext, we cache any found metadata information after
   * any initial load to lessen the workload of constant metadata loading on
   * each frame.
   */
  MetadataCache metadata_cache;

  /**
   * A buffer used internally while rendering the TUI to shrink the given video
   * frame to fit the rendered terminal frame space (if necessary).
   */
  PixelData scaling_buffer;
  
  /**
   * A field used to detect if the entire tui should be rerendered based on
   * a change in the size of tmedia's current frame.
   */
  Dim2 last_frame_dims{1, 1};

  /**
   * A field used to communicate between the TUI and the tmedia main loop on
   * what size the MediaFetcher instance should return frames.
   */
  Dim2 req_frame_dim{1, 1};
};


void render_tui(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs);

#endif
