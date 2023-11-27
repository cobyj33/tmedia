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

// class TMediaViewState {
//   PixelData frame;
//   VideoOutputMode vom;
//   ScalingAlgo scaling_algorithm;
//   bool fullscreen;
//   std::string ascii_display_chars;
// };

// class TMediaPlayerHandle {
//   private:
//     Playlist* playlist;
//     MediaFetcher* media;
//     AudioOut* audio_output;
//     double m_volume;
//     bool m_muted;
//     int refresh_rate_fps;

//   public:
//     TMediaPlayer(std::string source);

//     void run();

//     void start();
//     void stop();
//     void toggle_playback();

//     void skip();
//     void rewind();
    
//     bool playing();
//     double volume();
//     bool muted();
//     double duration();
//     double time(double current_system_time);

//     void mute();
//     void set_volume();
//     void seek(double media_time, double current_system_time);
// };

// struct TMediaState {
//   Playlist* playlist;
//   MediaFetcher* media;
//   AudioOut* audio_output;

//   bool quit;
//   PixelData frame;

//   VideoOutputMode vom;
//   bool fullscreen;
//   ScalingAlgo scaling_algorithm;
//   std::string ascii_display_chars;
// };

// typedef std::function<void(TMediaState*, double current_system_time)> TMediaCommand;

// TMediaCommand tmedia_toggle_playback_command();
// TMediaCommand exit_command();
// TMediaCommand toggle_shuffle_command();
// TMediaCommand toggle_mute_command();
// TMediaCommand toggle_loop_command();
// TMediaCommand skip_command();
// TMediaCommand rewind_command();
// TMediaCommand seek_command(double time);
// TMediaCommand seek_offset_command(double offset_time);
// TMediaCommand image_output_command(VideoOutputMode vom);

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
    void render_tui(const TMediaProgramState tmps);
};

void tmedia_handle_key(int key);


int tmedia(TMediaStartupState tmpd);

#endif