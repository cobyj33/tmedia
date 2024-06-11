#include <tmedia/tmedia.h>

#include <tmedia/cmdlpar/cmdlpar.h>
#include <tmedia/media/playlist.h>
#include <tmedia/util/formatting.h>
#include <tmedia/version.h>
#include <tmedia/util/defines.h>
#include <tmedia/image/scale.h>
#include <tmedia/ffmpeg/probe.h>
#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/ctnr/arraypairmap.hpp>

#include <vector>
#include <array>
#include <cstddef>
#include <map>
#include <functional>
#include <filesystem>
#include <iostream>
#include <charconv>
#include <stack>
#include <system_error>
#include <optional>

#include <fmt/format.h>
#include <natural_sort.hpp>

extern "C" {
  #include <curses.h>
  #include <miniaudio.h>
  #include <libavutil/log.h>
  #include <libavformat/version.h>
  #include <libavutil/version.h>
  #include <libavcodec/version.h>
  #include <libswresample/version.h>
  #include <libswscale/version.h>
}

namespace fs = std::filesystem;

// curses really thought it was a good idea
// to create macros named TRUE and FALSE...
#undef TRUE
#undef FALSE

// namespace tmedia {

// Perhaps the controls usage and help text could be generated dynamically
// later, so that the usage, help_text, and actual functionality are never
// out of sync.  
// as of now, I'd rather not write more code for something that's doable by
// hand.

const char* TMEDIA_CLI_USAGE = ""
"Usage: tmedia [OPTIONS] paths";

// When editing this string, make sure to edit the corresponding text in the
// top level README.md file
const char* TMEDIA_CONTROLS_USAGE = ""
  "---------------------------------CONTROLS---------------------------------\n"
  "Labels for this document: \n"
  "  (OST) - On Supported Terminals\n"
  "\n"
  "Video and Audio Controls\n"
  "- Space - Play and Pause\n"
  "- Up Arrow - Increase Volume 1%\n"
  "- Down Arrow - Decrease Volume 1%\n"
  "- Left Arrow - Skip Backward 5 Seconds\n"
  "- Right Arrow - Skip Forward 5 Seconds\n"
  "- Escape or Backspace or 'q' - Quit Program\n"
  "- '0' - Restart Playback\n"
  "- '1' through '9' - Skip To n/10 of the Media's Duration\n"
  "- 'L' - Switch looping type of playback\n"
  "- 'M' - Mute/Unmute Audio\n"
  "Video, Audio, and Image Controls\n"
  "- 'C' - Color Mode (OST)\n"
  "- 'G' - Gray Mode (OST)\n"
  "- 'B' - Display no Characters (OST) (in Color or Gray mode)\n"
  "- 'N' - Skip to Next Media File\n"
  "- 'P' - Rewind to Previous Media File\n"
  "- 'R' - Fully Refresh the Screen\n"
  "---------------------------------------------------------------------------";

// ! When editing this string, make sure to edit the corresponding text in the
// ! top-level README.md file.
// ! When documenting new CLI argument entries, make sure to also document the
// ! new CLI argument in doc/cli.md and the top-level README.md file.
const char* TMEDIA_CLI_ARGS_DESC = ""
  "-------------------------------CLI ARGUMENTS------------------------------\n"
  "\n"
  "Positional arguments:\n"
  "  paths                  The the paths to files or directories to be\n"
  "                         played. Multiple files will be played one\n"
  "                         after the other Directories will be expanded\n"
  "                         so any media files inside them will be played.\n"
  "\n"
  "\n"
  "Optional arguments:\n"
  "  Help and Versioning: \n"
  "  -h, --help             shows help message and exits \n"
  "  --help-cli             shows help message for cli options and exits \n"
  "  --help-controls        shows help message for tmedia controls and exits \n"
  "  -v, --version          prints version information and exits \n"
  "  --ffmpeg-version       Print the version of linked FFmpeg libraries \n"
  "  --curses-version       Print the version of linked Curses libraries\n"
  "  --fmt-version          Print the version of the fmt library\n"
  "  --miniaudio-version    Print the version of the miniaudio library\n"
  "  --lib-versions         Print the versions of third party libraries\n"
  "\n"
  "Video Output: \n"
  "  -c, --color            Play the video with color \n"
  "  -g, --gray             Play the video in grayscale \n"
  "  -b, --background       Do not show characters, only the background \n"
  "  -f, --fullscreen       Begin the player in fullscreen mode\n"
  "  --show-ctrl-info       Begin the player showing control info (Default Yes)\n"
  "  --hide-ctrl-info       Begin the player hiding control info (Default Yes)\n"
  "  --refresh-rate         Set the refresh rate of tmedia\n"
  "  --chars [STRING]       The displayed characters from darkest to lightest\n"
  "\n"
  "Audio Output: \n"
  "  --volume ([FLOAT] || [INT%]) Set initial volume ([0.0, 1.0] or [0%, 100%])\n"
  "  -m, --mute, --muted        Mute the audio playback \n"
  "\n"
  "Stream Control Commands: \n"
  "  --enable-audio-stream  (Default) Enable playback of audio streams\n"
  "  --enable-video-stream  (Default) Enable playback of video streams\n"
  "  --disable-audio-stream  Disable playback of audio streams\n"
  "  --disable-video-stream  Disable playback of video streams\n"
  "  :enable-audio-stream    Enable playback of audio streams for last path\n"
  "  :enable-video-stream    Enable playback of video streams for last path\n"
  "  :disable-audio-stream   Disable playback of audio streams for last path\n"
  "  :disable-video-stream   Disable playback of video streams for last path\n"
  "\n"
  "Playlist Controls: \n"
  "  --no-repeat            Do not repeat the playlist upon end\n"
  "  --repeat, --loop       Repeat the playlist upon playlist end\n"
  "  --repeat-one           Start the playlist looping the first media\n"
  "  -s, --shuffle          Shuffle the given playlist \n"
  "\n"
  "File Searching: \n"
  "  NOTE: all local (:) options override global options\n"
  "\n"
  "  -r, --recursive        Recurse child directories to find media files \n"
  "  --ignore-images        Ignore image files while searching directories\n"
  "  --ignore-video         Ignore video files while searching directories\n"
  "  --ignore-audio         Ignore audio files while searching directories\n"
  "  --probe, --no-probe    (Don't) probe all files\n"
  "  --repeat-paths [UINT]    Repeat all cli path arguments n times\n"
  "  :r, :recursive         Recurse child directories of the last listed path \n"
  "  :repeat-path [UINT]    Repeat the last given cli path argument n times\n"
  "  :ignore-images         Ignore image files when searching last listed path\n"
  "  :ignore-video          Ignore video files when searching last listed path\n"
  "  :ignore-audio          Ignore audio files when searching last listed path\n"
  "  :probe, :no-probe      (Don't) probe last file/directory\n"
  "---------------------------------------------------------------------------";

  /**
   * The idea of MediaPathSearchOptions and MediaPathLocalSearchOptions is that
   * each media item will either have global or local values assigned to it
   * (such as configuring to ignore images or videos).
   * Therefore, when the inheritable boolean is std::nullopt, we signal
   * to read the global value rather than any local value.
   * 
  */

  struct MediaPathSearchOptions {
    bool ignore_audio = false;
    bool ignore_video = false;
    bool ignore_images = false;
    bool recurse = false;
    bool probe = false;
    unsigned int num_reads = 1;
    std::array<bool, AVMEDIA_TYPE_NB> requested_streams = {true, true, false, false, false};
  };

  /**
   * Mirrors MediaPathSearchOptions, but all values are std::nullopt by
   * default and all values are compositions of std::optional. This
   * semantically means that all paths inherit global properties of the
   * command line by default.
  */
  struct MediaPathLocalSearchOptions {
    std::optional<bool> ignore_audio = std::nullopt;
    std::optional<bool> ignore_video = std::nullopt;
    std::optional<bool> ignore_images = std::nullopt;
    std::optional<bool> recurse = std::nullopt;
    std::optional<bool> probe = std::nullopt;
    std::optional<unsigned int> num_reads = std::nullopt;
    std::array<std::optional<bool>, AVMEDIA_TYPE_NB> requested_streams = {std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
  };

  bool resolve_inheritable_bool(std::optional<bool> ib, bool parent_bool);
  MediaPathSearchOptions resolve_path_search_options(MediaPathSearchOptions global, MediaPathLocalSearchOptions local);

  struct MediaPath {
    fs::path path;
    MediaPathLocalSearchOptions srch_opts;
    MediaPath() = default;
    MediaPath(std::string_view strv_path) : path(fs::path(strv_path)) {} 
    MediaPath(const fs::path& path) : path(path) {} 
    MediaPath(const MediaPath& o) = default;
    MediaPath(MediaPath&& o) : path(std::move(o.path)), srch_opts(o.srch_opts) {} 
  };

  struct CLIParseState {
    TMediaStartupState tmss;
    std::vector<MediaPath> paths;
    std::vector<std::string> argerrs;

    MediaPathSearchOptions srch_opts;
    bool colored = false;
    bool grayscale = false;
    bool background = false;
  };

  void resolve_cli_path(const fs::path& path,
                        MediaPathSearchOptions srch_opts,
                        std::vector<PlaylistItem>& resolved_paths);

  typedef std::function<void(CLIParseState&, const tmedia::CLIArg arg)> ArgParseFunc;
  typedef tmedia::ArrayPairMap<std::string_view, ArgParseFunc, 100, std::less<>> ArgParseMap;


  std::string missed_opt_err(tmedia::CLIArg arg, std::string_view invokation);
  template <typename BeginItr, typename EndItr>
  std::string missed_longopt_err(tmedia::CLIArg arg, std::string_view invokation, BeginItr begin, EndItr end);

  void print_help_text();
  void print_help_cli();
  void print_help_controls();

  #define TMEDIA_FMT_VERSION_MAJOR (FMT_VERSION / 10000)
  #define TMEDIA_FMT_VERSION_MINOR ((FMT_VERSION % 10000 - FMT_VERSION % 100) / 100)
  #define TMEDIA_FMT_VERSION_PATCH (FMT_VERSION % 100)

  // Early Exit Options
  void cli_arg_help(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_help_cli(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_help_controls(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_version(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ffmpeg_version(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_curses_version(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_fmt_version(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_miniaudio_version(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_lib_versions(CLIParseState& ps, const tmedia::CLIArg arg);

  // Global only options
  void cli_arg_background(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_background(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_chars(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_color(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_color(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_show_ctrl_info(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_show_ctrl_info(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_fullscreen(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_fullscreen(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_grayscale(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_grayscale(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_repeat(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_repeat(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_repeat_one(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_mute(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_mute(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_refresh_rate(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_shuffle(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_shuffle(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_volume(CLIParseState& ps, const tmedia::CLIArg arg);

  // Global versions of options that are either local or global
  void cli_arg_ignore_video_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_ignore_video_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_audio_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_ignore_audio_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_images_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_ignore_images_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_recurse_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_recurse_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_probe_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_probe_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_repeat_paths_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_enable_video_stream_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_enable_audio_stream_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_enable_video_stream_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_enable_audio_stream_global(CLIParseState& ps, const tmedia::CLIArg arg);

  // Local versions of options that are either local or global
  void cli_arg_ignore_video_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_ignore_video_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_audio_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_ignore_audio_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_images_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_ignore_images_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_recurse_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_recurse_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_probe_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_probe_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_repeat_path_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_enable_video_stream_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_enable_audio_stream_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_enable_video_stream_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_enable_audio_stream_local(CLIParseState& ps, const tmedia::CLIArg arg);


  TMediaCLIParseRes tmedia_parse_cli(int argc, char** argv) {
    CLIParseState ps;
    if (argc == 1) {
      print_help_text();
      return { ps.tmss, true, ps.argerrs };
    }

    std::vector<tmedia::CLIArg> parsed_cli;

    try {
      parsed_cli = tmedia::cli_parse(argc, argv, "",
      {"volume", "chars", "refresh-rate", "repeat-path", "repeat-paths"});
    } catch (const std::runtime_error& err) {
      ps.argerrs.push_back(fmt::format("[{}] Failed to parse CLI Arguments: {}", FUNCDINFO, err.what()));
      return { ps.tmss, false, ps.argerrs };
    }


    /*
    Note that a lot of options have aliases that aren't necessarily documented
    in the --help text and the doc/cli.md file, such as "--shuffled".
    These mostly exist for a better user experience, as "--shuffled" obviously
    means the same thing as "--shuffle" to a user. Another good example is 
    --gray and --grey.
    */

    static const ArgParseMap short_exiting_opt_map{
      {"h", cli_arg_help},
      {"v", cli_arg_version}
    };

    static const ArgParseMap long_exiting_opt_map{
      {"help", cli_arg_help},
      {"help-cli", cli_arg_help_cli},
      {"help-controls", cli_arg_help_controls},
      {"version", cli_arg_version},
      {"ffmpeg-version", cli_arg_ffmpeg_version},
      {"curses-version", cli_arg_curses_version},
      {"fmt-version", cli_arg_fmt_version},
      {"miniaudio-version", cli_arg_miniaudio_version},
      {"ma-version", cli_arg_miniaudio_version},
      {"dep-version", cli_arg_lib_versions},
      {"dep-versions", cli_arg_lib_versions},
      {"lib-version", cli_arg_lib_versions},
      {"lib-versions", cli_arg_lib_versions},
    };

    static const ArgParseMap short_global_argmap{
      {"c", cli_arg_color},
      {"b", cli_arg_background},
      {"g", cli_arg_grayscale},
      {"m", cli_arg_mute},
      {"f", cli_arg_fullscreen},
      {"s", cli_arg_shuffle},
      {"r", cli_arg_recurse_global},
    };

    static const ArgParseMap long_global_argmap{
      {"no-repeat", cli_arg_no_repeat},
      {"no-loop", cli_arg_no_repeat},
      {"repeat", cli_arg_repeat},
      {"loop", cli_arg_repeat},
      {"repeat-one", cli_arg_repeat_one},
      {"loop-one", cli_arg_repeat_one},
      {"volume", cli_arg_volume},
      {"shuffle", cli_arg_shuffle},
      {"shuffled", cli_arg_shuffle},
      {"no-shuffle", cli_arg_no_shuffle},
      {"no-shuffled", cli_arg_no_shuffle},
      {"refresh-rate", cli_arg_refresh_rate},
      {"chars", cli_arg_chars},

      {"color", cli_arg_color},
      {"colour", cli_arg_color},
      {"colored", cli_arg_color},
      {"coloured", cli_arg_color},
      {"no-color", cli_arg_no_color},
      {"no-colour", cli_arg_no_color},
      {"no-colored", cli_arg_no_color},
      {"no-coloured", cli_arg_no_color},

      {"gray", cli_arg_grayscale},
      {"grey", cli_arg_grayscale},
      {"greyscale", cli_arg_grayscale},
      {"grayscale", cli_arg_grayscale},
      {"no-gray", cli_arg_no_grayscale},
      {"no-grey", cli_arg_no_grayscale},
      {"no-greyscale", cli_arg_no_grayscale},
      {"no-grayscale", cli_arg_no_grayscale},

      {"background", cli_arg_background},
      {"no-background", cli_arg_no_background},

      {"mute", cli_arg_mute},
      {"muted", cli_arg_mute},
      {"no-mute", cli_arg_no_mute},
      {"no-muted", cli_arg_no_mute},
      {"no-muted", cli_arg_no_mute},

      {"fullscreen", cli_arg_fullscreen},
      {"no-fullscreen", cli_arg_no_fullscreen},

      {"show-ctrl-info", cli_arg_show_ctrl_info},
      {"show-control-info", cli_arg_show_ctrl_info},
      {"no-show-ctrl-info", cli_arg_no_show_ctrl_info},
      {"no-show-control-info", cli_arg_no_show_ctrl_info},
      {"hide-ctrl-info", cli_arg_no_show_ctrl_info},
      {"hide-control-info", cli_arg_no_show_ctrl_info},

      {"enable-video-stream", cli_arg_enable_video_stream_global},
      {"enable-audio-stream", cli_arg_enable_audio_stream_global},
      {"no-enable-video-stream", cli_arg_no_enable_video_stream_global},
      {"no-enable-audio-stream", cli_arg_no_enable_audio_stream_global},
      {"disable-video-stream", cli_arg_no_enable_video_stream_global},
      {"disable-audio-stream", cli_arg_no_enable_audio_stream_global},

      // path searching opts
      {"ignore-audio", cli_arg_ignore_audio_global},
      {"ignore-audios", cli_arg_ignore_audio_global},
      {"no-ignore-audio", cli_arg_no_ignore_audio_global},
      {"no-ignore-audios", cli_arg_no_ignore_audio_global},
      {"dont-ignore-audio", cli_arg_no_ignore_audio_global},
      {"dont-ignore-audios", cli_arg_no_ignore_audio_global},
      
      {"ignore-video", cli_arg_ignore_video_global},
      {"ignore-videos", cli_arg_ignore_video_global},
      {"no-ignore-video", cli_arg_no_ignore_video_global},
      {"no-ignore-videos", cli_arg_no_ignore_video_global},
      {"dont-ignore-video", cli_arg_no_ignore_video_global},
      {"dont-ignore-videos", cli_arg_no_ignore_video_global},
      
      {"ignore-image", cli_arg_ignore_images_global},
      {"ignore-images", cli_arg_ignore_images_global},
      {"no-ignore-image", cli_arg_no_ignore_images_global},
      {"no-ignore-images", cli_arg_no_ignore_images_global},
      {"dont-ignore-image", cli_arg_no_ignore_images_global},
      {"dont-ignore-images", cli_arg_no_ignore_images_global},

      {"recurse", cli_arg_recurse_global},
      {"recursive", cli_arg_recurse_global},
      {"no-recurse", cli_arg_no_recurse_global},
      {"no-recursive", cli_arg_no_recurse_global},
      {"dont-recurse", cli_arg_no_recurse_global},
      {"dont-recursive", cli_arg_no_recurse_global},

      {"probe", cli_arg_probe_global},
      {"no-probe", cli_arg_no_probe_global},
      {"dont-probe", cli_arg_no_probe_global},
      
      {"repeat-paths", cli_arg_repeat_paths_global},
      {"repeat-path", cli_arg_repeat_paths_global},
    };

    static const ArgParseMap short_local_argmap{
      {"r", cli_arg_recurse_local}
    };

    static const ArgParseMap long_local_argmap{
      {"enable-video-stream", cli_arg_enable_video_stream_local},
      {"enable-audio-stream", cli_arg_enable_audio_stream_local},
      {"no-enable-video-stream", cli_arg_no_enable_video_stream_local},
      {"no-enable-audio-stream", cli_arg_no_enable_audio_stream_local},
      {"disable-video-stream", cli_arg_no_enable_video_stream_local},
      {"disable-audio-stream", cli_arg_no_enable_audio_stream_local},

      {"ignore-audio", cli_arg_ignore_audio_local},
      {"ignore-audios", cli_arg_ignore_audio_local},
      {"no-ignore-audio", cli_arg_no_ignore_audio_local},
      {"no-ignore-audios", cli_arg_no_ignore_audio_local},
      {"dont-ignore-audio", cli_arg_no_ignore_audio_local},
      {"dont-ignore-audios", cli_arg_no_ignore_audio_local},
      
      {"ignore-video", cli_arg_ignore_video_local},
      {"ignore-videos", cli_arg_ignore_video_local},
      {"no-ignore-video", cli_arg_no_ignore_video_local},
      {"no-ignore-videos", cli_arg_no_ignore_video_local},
      {"dont-ignore-video", cli_arg_no_ignore_video_local},
      {"dont-ignore-videos", cli_arg_no_ignore_video_local},
      
      {"ignore-image", cli_arg_ignore_images_local},
      {"ignore-images", cli_arg_ignore_images_local},
      {"no-ignore-image", cli_arg_no_ignore_images_local},
      {"no-ignore-images", cli_arg_no_ignore_images_local},
      {"dont-ignore-image", cli_arg_no_ignore_images_local},
      {"dont-ignore-images", cli_arg_no_ignore_images_local},

      {"recurse", cli_arg_recurse_local},
      {"recursive", cli_arg_recurse_local},
      {"no-recurse", cli_arg_no_recurse_local},
      {"no-recursive", cli_arg_no_recurse_local},
      {"dont-recurse", cli_arg_no_recurse_local},
      {"dont-recursive", cli_arg_no_recurse_local},

      {"probe", cli_arg_probe_local},
      {"no-probe", cli_arg_no_probe_local},
      {"dont-probe", cli_arg_no_probe_local},

      {"repeat-path", cli_arg_repeat_path_local},
      {"repeat-paths", cli_arg_repeat_path_local},
    };

    for (const tmedia::CLIArg& arg : parsed_cli) {
      switch (arg.arg_type) {
        case tmedia::CLIArgType::POSITIONAL: {
          try {
            ps.paths.push_back(MediaPath(arg.value));
          } catch (const std::exception& e) { // fs::path creation failed
            ps.argerrs.push_back(fmt::format("[{}]: Failed to create Media "
              "Path from cli argument #{} ({}): {}",
              FUNCDINFO, arg.argi, arg.value, e.what()));
          }
        } break;
        case tmedia::CLIArgType::OPTION: {
          if (arg.prefix == "-") {

            if (short_exiting_opt_map.count(arg.value)) {
              short_exiting_opt_map.at(arg.value)(ps, arg);
              return { std::move(ps.tmss), true, std::move(ps.argerrs) };
            } else if (short_global_argmap.count(arg.value)) {
              short_global_argmap.at(arg.value)(ps, arg);
            } else {
              ps.argerrs.push_back(fmt::format("[{}] {}", FUNCDINFO,
                missed_opt_err(arg, argv[0])));
            }

          } else if (arg.prefix == "--") {

            if (long_exiting_opt_map.count(arg.value)) {
              long_exiting_opt_map.at(arg.value)(ps, arg);
              return { std::move(ps.tmss), true, std::move(ps.argerrs) };
            } else if (long_global_argmap.count(arg.value)) {
              long_global_argmap.at(arg.value)(ps, arg);
            } else {
              ps.argerrs.push_back(fmt::format("[{}] {}", FUNCDINFO,
                missed_longopt_err(arg, argv[0],
                long_global_argmap.kbegin(), long_global_argmap.kend())));
            }

          } else if (arg.prefix == ":") {

            if (short_local_argmap.count(arg.value)) {
              short_local_argmap.at(arg.value)(ps, arg);
            } else if (long_local_argmap.count(arg.value)) {
              long_local_argmap.at(arg.value)(ps, arg);
            } else {
              ps.argerrs.push_back(fmt::format("[{}] {}", FUNCDINFO,
                missed_longopt_err(arg, argv[0],
                long_local_argmap.kbegin(), long_local_argmap.kend())));
            }

          }

        } break;
      }
    }

    if (ps.colored)
      ps.tmss.vom = ps.background ? VidOutMode::COLOR_BG : VidOutMode::COLOR;
    else if (ps.grayscale)
      ps.tmss.vom = ps.background ? VidOutMode::GRAY_BG : VidOutMode::GRAY;

    if (ps.paths.size() == 0UL) {
      ps.argerrs.push_back(fmt::format("[{}] No paths entered", FUNCDINFO));
    }

    if (ps.srch_opts.requested_streams[AVMEDIA_TYPE_VIDEO] == false &&
        ps.srch_opts.requested_streams[AVMEDIA_TYPE_AUDIO] == false) {
      ps.argerrs.push_back(fmt::format("[{}] Disabled both audio and video "
        "streams globally, effectively disabling all media files from being "
        "read. Please make sure that not both --disable-video-stream and "
        "--disable-audio-stream are present at once on the "
        "command line.", FUNCDINFO));
    }

    for (const MediaPath& path : ps.paths) {
      MediaPathSearchOptions resolved_srch_opts = resolve_path_search_options(ps.srch_opts, path.srch_opts);
      for (unsigned int i = 0; i < resolved_srch_opts.num_reads; i++) {
        try {
          resolve_cli_path(path.path, resolved_srch_opts, ps.tmss.media_files);
        } catch (const std::runtime_error& err) {
          ps.argerrs.push_back(fmt::format("[{}] Failed to resolve CLI Path "
          "{}: {}", FUNCDINFO, path.path.string(), err.what()));
        }
      }
    }

    if (ps.tmss.media_files.size() == 0UL) {
      ps.argerrs.push_back(fmt::format("[{}] No media files found.", FUNCDINFO));
    }
    
    TMediaCLIParseRes res;
    res.tmss = std::move(ps.tmss);
    res.exited = false;
    res.errors = std::move(ps.argerrs);
    return res;
  }

  void print_help_text() {
    std::cout << TMEDIA_CLI_USAGE << '\n' << std::endl;
    std::cout << TMEDIA_CONTROLS_USAGE << '\n' << std::endl;
    std::cout << TMEDIA_CLI_ARGS_DESC << std::endl;
  }

  void cli_arg_help(CLIParseState& ps, const tmedia::CLIArg arg) {
    print_help_text();
    (void)ps; (void)arg;
  }

  void cli_arg_help_controls(CLIParseState& ps, const tmedia::CLIArg arg) {
    std::cout << TMEDIA_CONTROLS_USAGE << std::endl;
    (void)ps; (void)arg;
  }

  void cli_arg_help_cli(CLIParseState& ps, const tmedia::CLIArg arg) {
    std::cout << TMEDIA_CLI_ARGS_DESC << std::endl;
    (void)ps; (void)arg;
  }

  void cli_arg_version(CLIParseState& ps, const tmedia::CLIArg arg) {
    std::cout << TMEDIA_VERSION << std::endl;
    (void)ps; (void)arg;
  }

  void print_fmt_version() {
    std::cout << "{fmt} version: "
              <<  TMEDIA_FMT_VERSION_MAJOR << "."
              << TMEDIA_FMT_VERSION_MINOR << "."
              << TMEDIA_FMT_VERSION_PATCH << std::endl;
  }

  void print_miniaudio_version() {
    std::cout << "miniaudio version: " << MA_VERSION_STRING << std::endl;
  }

  void print_ffmpeg_version() {
    std::cout << "libavformat: " << LIBAVFORMAT_VERSION_MAJOR << ":" <<
                                 LIBAVFORMAT_VERSION_MINOR << ":" <<
                                 LIBAVFORMAT_VERSION_MICRO << std::endl;
    std::cout << "libavcodec: " << LIBAVCODEC_VERSION_MAJOR << ":" <<
                                 LIBAVCODEC_VERSION_MINOR << ":" <<
                                 LIBAVCODEC_VERSION_MICRO << std::endl;
    std::cout << "libavutil: " << LIBAVUTIL_VERSION_MAJOR << ":" <<
                                 LIBAVUTIL_VERSION_MINOR << ":" <<
                                 LIBAVUTIL_VERSION_MICRO << std::endl;
    std::cout << "libswresample: " << LIBSWRESAMPLE_VERSION_MAJOR << ":" <<
                                 LIBSWRESAMPLE_VERSION_MINOR << ":" <<
                                 LIBSWRESAMPLE_VERSION_MICRO << std::endl;
    std::cout << "libswscale: " << LIBSWSCALE_VERSION_MAJOR << ":" <<
                                 LIBSWSCALE_VERSION_MINOR << ":" <<
                                 LIBSWSCALE_VERSION_MICRO << std::endl;
  }

  void print_curses_version() {
    std::cout << curses_version() << std::endl;
  }

  void cli_arg_ffmpeg_version(CLIParseState& ps, const tmedia::CLIArg arg) {
    print_ffmpeg_version();
    (void)ps; (void)arg;
  }

  void cli_arg_curses_version(CLIParseState& ps, const tmedia::CLIArg arg) {
    print_curses_version();
    (void)ps; (void)arg;
  }

  void cli_arg_fmt_version(CLIParseState& ps, const tmedia::CLIArg arg) {
    print_fmt_version();
    (void)ps; (void)arg;
  }

  void cli_arg_miniaudio_version(CLIParseState& ps, const tmedia::CLIArg arg) {
    print_miniaudio_version();
    (void)ps; (void)arg;
  }

  void cli_arg_lib_versions(CLIParseState& ps, const tmedia::CLIArg arg) {
    print_curses_version();
    print_ffmpeg_version();
    print_fmt_version();
    print_miniaudio_version();
    (void)ps; (void)arg;
  }

  void cli_arg_background(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.background = true;
    (void)arg;
  }

  void cli_arg_no_background(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.background = false;
    (void)arg;
  }

  void cli_arg_chars(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.ascii_display_chars = arg.param;
    (void)arg;
  }

  void cli_arg_color(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.colored = true;
    (void)arg;
  }

  void cli_arg_no_color(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.colored = false;
    (void)arg;
  }

  void cli_arg_show_ctrl_info(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.show_ctrl_info = true;
    (void)arg;
  }

  void cli_arg_no_show_ctrl_info(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.show_ctrl_info = false;
    (void)arg;
  }

  void cli_arg_fullscreen(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.fullscreen = true;
    (void)arg;
  }

  void cli_arg_no_fullscreen(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.fullscreen = false;
    (void)arg;
  }

  void cli_arg_grayscale(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.grayscale = true;
    (void)arg;
  }

  void cli_arg_no_grayscale(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.grayscale = false;
    (void)arg;
  }

  void cli_arg_no_repeat(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.loop_type = LoopType::NO_LOOP;
    (void)arg;
  }

  void cli_arg_repeat(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.loop_type = LoopType::REPEAT;
    (void)arg;
  }

  void cli_arg_repeat_one(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.loop_type = LoopType::REPEAT_ONE;
    (void)arg;
  }

  void cli_arg_mute(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.muted = true;
    (void)arg;
  }

  void cli_arg_no_mute(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.muted = false;
    (void)arg;
  }

  void cli_arg_refresh_rate(CLIParseState& ps, const tmedia::CLIArg arg) {
    int res = 24;
    
    try {
      res = strtoi32(arg.param);
      if (res <= 0) {
        ps.argerrs.push_back(fmt::format("[{}] refresh rate must be greater "
        "than 0. (got {})", FUNCDINFO, res));
      }
    } catch (const std::runtime_error& err) {
      ps.argerrs.push_back(fmt::format("[{}] Could not parse param {} as "
      "integer: \n\t{}", FUNCDINFO, arg.param, err.what()));
    }

    ps.tmss.refresh_rate_fps = res;
  }

  void cli_arg_shuffle(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.shuffled = true;
    (void)arg;
  }

  void cli_arg_no_shuffle(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.shuffled = false;
    (void)arg;
  }

  void cli_arg_volume(CLIParseState& ps, const tmedia::CLIArg arg) {
    double volume = 1.0;
    try {
      volume = parse_percentage(arg.param);
      if (volume >= 0.0 && volume <= 1.0) {
        ps.tmss.volume = volume;
      } else {
        ps.argerrs.push_back(fmt::format("[{}] Volume out of bounds "
        " [0.0, 1.0] (got {})", FUNCDINFO, volume));
      }
    } catch (const std::runtime_error& err) {
      ps.argerrs.push_back(fmt::format("[{}] Could not parse parameter {} "
      "as a percentage value: \n\t{}", FUNCDINFO, arg.param, err.what()));
    }
  }

  void cli_arg_ignore_video_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.ignore_video = true;
    (void)arg;
  }

  void cli_arg_no_ignore_video_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.ignore_video = false;
    (void)arg;
  }

  void cli_arg_ignore_audio_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.ignore_audio = true;
    (void)arg;
  }

  void cli_arg_no_ignore_audio_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.ignore_audio = false;
    (void)arg;
  }

  void cli_arg_ignore_images_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.ignore_images = true;
    (void)arg;
  }

  void cli_arg_no_ignore_images_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.ignore_images = false;
    (void)arg;
  }


  void cli_arg_recurse_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.recurse = true;
    (void)arg;
  }

  void cli_arg_no_recurse_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.recurse = false;
    (void)arg;
  }

  void cli_arg_probe_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.probe = true;
    (void)arg;
  }

  void cli_arg_no_probe_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.probe = false;
    (void)arg;
  }

  void cli_arg_repeat_paths_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    try {
      int repeats = strtoi32(arg.param);
      if (repeats >= 0) {
        ps.srch_opts.num_reads = repeats + 1; // plus one for the additional normal read
      } else {
        ps.argerrs.push_back(fmt::format("[{}] Could not read unsigned integer "
        "from {}{} at index ${}: Received a negative number '{}'", FUNCDINFO,
        arg.prefix, arg.param, arg.argi, repeats));
      }
    } catch (const std::runtime_error& err) {
      ps.argerrs.push_back(fmt::format("[{}] Could not read unsigned integer "
      "from {}{} at index #{}: {}", FUNCDINFO, arg.prefix, arg.param,
      arg.argi, err.what()));
    }
    (void)arg;
  }

  void cli_arg_enable_video_stream_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.requested_streams[AVMEDIA_TYPE_VIDEO] = true;
    (void)arg;
  }

  void cli_arg_enable_audio_stream_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.requested_streams[AVMEDIA_TYPE_AUDIO] = true;
    (void)arg;
  }
  
  void cli_arg_no_enable_video_stream_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.requested_streams[AVMEDIA_TYPE_VIDEO] = false;
    (void)arg;
  }

  void cli_arg_no_enable_audio_stream_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.requested_streams[AVMEDIA_TYPE_AUDIO] = false;
    (void)arg;
  }


  void cli_arg_ignore_video_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_video = true;
    (void)arg;
  }

  void cli_arg_no_ignore_video_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_video = false;
    (void)arg;
  }

  void cli_arg_ignore_audio_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_audio = true;
    (void)arg;
  }

  void cli_arg_no_ignore_audio_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_audio = false;
    (void)arg;
  }

  void cli_arg_ignore_images_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_images = true;
    (void)arg;
  }

  void cli_arg_no_ignore_images_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_images = false;
    (void)arg;
  }

  void cli_arg_recurse_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.recurse = true;
    (void)arg;
  }

  void cli_arg_no_recurse_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.recurse = false;
    (void)arg;
  }

  void cli_arg_probe_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.probe = true;
    (void)arg;
  }

  void cli_arg_no_probe_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.probe = false;
    (void)arg;
  }

  void cli_arg_enable_video_stream_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.requested_streams[AVMEDIA_TYPE_VIDEO] = true;
    (void)arg;
  }

  void cli_arg_enable_audio_stream_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.requested_streams[AVMEDIA_TYPE_AUDIO] = true;
    (void)arg;
  }

  void cli_arg_no_enable_video_stream_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.requested_streams[AVMEDIA_TYPE_VIDEO] = false;
    (void)arg;
  }

  void cli_arg_no_enable_audio_stream_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL)
      ps.paths[ps.paths.size() - 1UL].srch_opts.requested_streams[AVMEDIA_TYPE_AUDIO] = false;
    (void)arg;
  }

  void cli_arg_repeat_path_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() == 0UL) return;

    try {
      int repeats = strtoi32(arg.param);
      if (repeats >= 0) {
        ps.paths[ps.paths.size() - 1UL].srch_opts.num_reads = repeats + 1; // plus one for the additional normal read
      } else {
        ps.argerrs.push_back(fmt::format("[{}] Could not read unsigned integer "
        "from {}{} at index ${}: Received a negative number '{}'", FUNCDINFO,
        arg.prefix, arg.param, arg.argi, repeats));
      }
    } catch (const std::runtime_error& err) {
      ps.argerrs.push_back(fmt::format("[{}] Could not read unsigned integer "
      "from {}{} at index #{}: {}", FUNCDINFO, arg.prefix, arg.param,
      arg.argi, err.what()));
    }
    (void)arg;
  }

  bool resolve_inheritable_bool(std::optional<bool> ib, bool parent_bool) {
    return ib == std::nullopt ? parent_bool : (ib.value() == true);
  }

  MediaPathSearchOptions resolve_path_search_options(MediaPathSearchOptions global, MediaPathLocalSearchOptions local) {
    MediaPathSearchOptions res;
    res.ignore_video = resolve_inheritable_bool(local.ignore_video, global.ignore_video);
    res.ignore_audio = resolve_inheritable_bool(local.ignore_audio, global.ignore_audio);
    res.ignore_images = resolve_inheritable_bool(local.ignore_images, global.ignore_images);
    res.recurse = resolve_inheritable_bool(local.recurse, global.recurse);
    res.probe = resolve_inheritable_bool(local.probe, global.probe);
    res.num_reads = local.num_reads.has_value() ? (*local.num_reads) : global.num_reads;
    for (std::size_t i = 0; i < global.requested_streams.size(); i++) {
      res.requested_streams[i] = resolve_inheritable_bool(local.requested_streams[i], global.requested_streams[i]);
    }

    return res;
  }

  bool test_media_file(const fs::path& path, MediaPathSearchOptions search_opts) {
    std::optional<MediaType> media_type = search_opts.probe ?
                                          media_type_probe(path) :
                                          media_type_from_path(path);
    
    if (!media_type.has_value()) return false;
    
    if (media_type) {
      switch (*media_type) {
        case MediaType::VIDEO: return !search_opts.ignore_video &&
          (search_opts.requested_streams[AVMEDIA_TYPE_VIDEO] ||
          search_opts.requested_streams[AVMEDIA_TYPE_AUDIO]);
        case MediaType::AUDIO: return !search_opts.ignore_audio &&
          search_opts.requested_streams[AVMEDIA_TYPE_AUDIO];
        case MediaType::IMAGE: return !search_opts.ignore_images &&
          search_opts.requested_streams[AVMEDIA_TYPE_VIDEO];
      }
    }

    return false; // unreachable
  }


  void resolve_cli_path(const fs::path& path, MediaPathSearchOptions srch_opts, std::vector<PlaylistItem>& resolved_paths) {
    std::stack<fs::path> to_search;
    to_search.push(path);
    std::error_code ec;

    while (!to_search.empty()) {
      const fs::path curr = std::move(to_search.top());
      to_search.pop();

      if (!fs::exists(curr, ec)) {
        throw std::runtime_error(fmt::format("[{}] Cannot open nonexistent "
        "path: {}", FUNCDINFO, path.c_str()));
      }

      if (fs::is_directory(curr, ec)) {
        std::vector<std::string> media_file_paths;
        std::vector<std::string> directory_paths;

        for (const fs::directory_entry& entry : fs::directory_iterator(curr)) {
          if (fs::is_directory(entry.path(), ec) && srch_opts.recurse) {
            directory_paths.push_back(entry.path().string());
          } else if (fs::is_regular_file(entry.path()) && test_media_file(entry.path(), srch_opts)) {
            media_file_paths.push_back(entry.path().string());
          }
        }

        SI::natural::sort(media_file_paths);
        SI::natural::sort(directory_paths);

        for (auto&& media_file_path : media_file_paths) {
          resolved_paths.push_back(PlaylistItem(std::move(media_file_path), srch_opts.requested_streams));
        }

        // Since we are pushing onto a stack for searching, where
        // the first directory pushed will be read last, we have to push
        // the sorted directories in the reverse order so that they are
        // read in the correct order on later iterations.
        for (auto it = directory_paths.rbegin(); it != directory_paths.rend(); it++) {
          to_search.push(std::move(*it));
        }
      } else if (fs::is_regular_file(curr) && test_media_file(curr, srch_opts)) {
        resolved_paths.push_back(PlaylistItem(curr, srch_opts.requested_streams));
      }
    }
  }

unsigned int lev_within(std::string_view a, std::string_view b, unsigned int max_dis, unsigned int curr_dis) {
  while (a.length() > 0 || b.length() > 0) {
    if (b.length() == 0) {
      return curr_dis + a.length();
    } else if (a.length() == 0) {
      return curr_dis + b.length();
    } else if (a[0] == b[0]) {
      a = a.substr(1);
      b = b.substr(1);
    } else {
      if (curr_dis + 1 > max_dis)
        return max_dis + 1;

      std::string_view taila = a.substr(1);
      std::string_view tailb = b.substr(1);
      return std::min(lev_within(taila, b, max_dis, curr_dis + 1),
        std::min(lev_within(a, tailb, max_dis, curr_dis + 1),
        lev_within(taila, tailb, max_dis, curr_dis + 1)));
    }
  }

  return curr_dis;
}

unsigned int rank_opt_diff(std::string_view a, std::string_view b, unsigned int max_dis) {
  return lev_within(a, b.substr(0, a.length()), max_dis, 0);
}


std::string missed_opt_err(tmedia::CLIArg opt, std::string_view invokation) {
  return fmt::format("Unrecognized "
    "option: '{}{}'. Use the '{} --help' command to see all "
    "available cli options", opt.prefix, opt.value, invokation);
}

struct LevenshteinMatch {
  std::string_view strv;
  unsigned int distance;
};

template <typename BeginItr, typename EndItr>
std::string missed_longopt_err(tmedia::CLIArg opt, std::string_view invokation, BeginItr begin, EndItr end) {
  static constexpr unsigned int MAX_MATCHES = 3;
  static constexpr int LEV_CLI_SIMILAR_ENOUGH = 2;

  std::array<LevenshteinMatch, MAX_MATCHES> matches;
  unsigned int nb_matches = 0; // in range from 0 to MAX_MATCHES inclusive

  for (auto itr = begin; itr != end; itr++) {
    const unsigned int distance = rank_opt_diff(opt.value, *itr, LEV_CLI_SIMILAR_ENOUGH);
    if (distance > LEV_CLI_SIMILAR_ENOUGH) continue;

    LevenshteinMatch match = { *itr, distance };
    unsigned int insert_pos = 0;
    while(insert_pos < nb_matches && matches[insert_pos].distance < distance) {
      insert_pos++;
    }

    if (insert_pos < MAX_MATCHES) {
      nb_matches = std::min(MAX_MATCHES, nb_matches + 1);
      for (unsigned int i = nb_matches - 1; i > insert_pos; i--) {
        matches[i] = matches[i - 1];
      }
      matches[insert_pos] = match;
    }
  }

  if (nb_matches == 0) {
    return missed_opt_err(opt, invokation);
  } else {
    std::string err = fmt::format("Unrecognized option: '{}{}'. ",
      opt.prefix, opt.value);

    err += "Similar options: ";
    for (unsigned int i = 0; i < nb_matches; i++) {
      err += fmt::format("'{}{}'{}", opt.prefix, matches[i].strv,
        (i == nb_matches - 1) ? ". " : ", ");
    }
    err += fmt::format("Use '{} --help' to see all "
      "cli options", invokation);
    return err;
  }
}

// };
