#include "tmedia.h"

#include "cli_iter.h"
#include "playlist.h"
#include "formatting.h"
#include "version.h"
#include "ascii.h"
#include "scale.h"
#include "probe.h"
#include "boiler.h"
#include "pixeldata.h"
#include "funcmac.h"

#include <natural_sort.hpp>

#include <vector>
#include <cstddef>
#include <map>
#include <functional>
#include <filesystem>
#include <iostream>
#include <charconv>
#include <stack>

#include <fmt/format.h>

extern "C" {
  #include <curses.h>
  #include <libavutil/log.h>
  #include <libavformat/version.h>
  #include <libavutil/version.h>
  #include <libavcodec/version.h>
  #include <libswresample/version.h>
  #include <libswscale/version.h>
}

#undef TRUE
#undef FALSE

// namespace tmedia {

  const char* help_text = "Usage: tmedia OPTIONS paths\n"
  "\n"
  "CONTROLS:\n"
  "\n"
  "  Video and Audio Controls\n"
  "    - Space                   Play and Pause\n"
  "    - Up Arrow                Increase Volume 1%\n"
  "    - Down Arrow              Decrease Volume 1%\n"
  "    - Left Arrow              Skip Backward 5 Seconds\n"
  "    - Right Arrow             Skip Forward 5 Seconds\n"
  "    - Esc, Backspace or 'q'   Quit Program\n"
  "    - '0'                     Restart Playback\n"
  "    - '1' through '9'         Skip To n/10 of the Media's Duration\n"
  "    - 'L'                     Switch looping type of playback\n"
  "    - 'M'                     Mute/Unmute Audio\n"
  "  Video, Audio, and Image Controls\n"
  "    - 'C'    Display Color (on supported terminals)\n"
  "    - 'G'    Display Grayscale (on supported terminals)\n"
  "    - 'B'    Display Spaces (on supported terms & while colored/grayscale)\n"
  "    - 'N'    Skip to Next Media File\n"
  "    - 'P'    Rewind to Previous Media File\n"
  "    - 'R'    Fully Refresh the Screen\n"
  "\n"
  "ARGUMENTS:\n"
  "\n"
  "  Positional arguments:\n"
  "    paths                  The the paths to files or directories to be\n"
  "                           played. Multiple files will be played one\n"
  "                           after the other Directories will be expanded\n"
  "                           so any media files inside them will be played.\n"
  "\n"
  "\n"
  "  Optional arguments:\n"
  "    Help and Versioning: \n"
  "    -h, --help             shows help message and exits \n"
  "    -v, --version          prints version information and exits \n"
  "    --ffmpeg-version       Print the version of linked FFmpeg libraries \n"
  "    --curses-version       Print the version of linked Curses libraries\n"
  "\n"
  "  Video Output: \n"
  "    -c, --color            Play the video with color \n"
  "    -g, --gray             Play the video in grayscale \n"
  "    -b, --background       Do not show characters, only the background \n"
  "    -f, --fullscreen       Begin the player in fullscreen mode\n"
  "    --refresh-rate         Set the refresh rate of tmedia\n"
  "    --chars [STRING]       The displayed characters from darkest to lightest\n"
  "\n"
  "  Audio Output: \n"
  "    --volume [FLOAT] || [INT%] Set initial volume [0.0, 1.0] or [0%, 100%] \n"
  "    -m, --mute, --muted        Mute the audio playback \n"

  "  Playlist Controls: \n"
  "    --no-repeat            Do not repeat the playlist upon end\n"
  "    --repeat, --loop       Repeat the playlist upon playlist end\n"
  "    --repeat-one           Start the playlist looping the first media\n"
  "    -s, --shuffle          Shuffle the given playlist \n"
  "\n"
  "  File Searching: \n"
  "    -r, --recursive        Recurse child directories to find media files \n"
  "    --ignore-images        Ignore image files while searching directories\n"
  "    --ignore-video         Ignore video files while searching directories\n"
  "    --ignore-audio         Ignore audio files while searching directories\n"
  "    :r, :recursive         Recurse child directories of the last listed path \n"
  "    :ignore-images         Ignore image files when searching last listed path\n"
  "    :ignore-video          Ignore video files when searching last listed path\n"
  "    :ignore-audio          Ignore audio files when searching last listed path\n";

  /**
   * The idea of inheritable boolean is that each media item will either have
   * global or local values assigned to it (such as configuring to ignore images
   * or videos). Therefore, when the inheritable boolean is INHERIT, we signal
   * to read the global value rather than any local value.
  */
  enum class InheritableBoolean {
    FALSE = 0,
    TRUE = 1,
    INHERIT = -1
  };

  struct MediaPathSearchOptions {
    bool ignore_audio = false;
    bool ignore_video = false;
    bool ignore_images = false;
    bool recurse = false;
  };

  struct MediaPathLocalSearchOptions {
    InheritableBoolean ignore_audio = InheritableBoolean::INHERIT;
    InheritableBoolean ignore_video = InheritableBoolean::INHERIT;
    InheritableBoolean ignore_images = InheritableBoolean::INHERIT;
    InheritableBoolean recurse = InheritableBoolean::INHERIT;
  };

  bool resolve_inheritable_bool(InheritableBoolean ib, bool parent_bool);
  MediaPathSearchOptions resolve_path_search_options(MediaPathSearchOptions global, MediaPathLocalSearchOptions local);

  struct MediaPath {
    std::filesystem::path path;
    MediaPathLocalSearchOptions srch_opts;
    MediaPath(std::string_view path) : path(std::move(std::filesystem::path(path))) {} 
    MediaPath(const std::filesystem::path& path) : path(path) {} 
    MediaPath(MediaPath&& o) : path(std::move(o.path)), srch_opts(o.srch_opts) {} 
  };

  struct CLIParseState {
    TMediaStartupState tmss;
    std::vector<MediaPath> paths;

    MediaPathSearchOptions srch_opts;
    bool colored = false;
    bool grayscale = false;
    bool background = false;
  };

  void resolve_cli_path(const std::filesystem::path& path,
                        MediaPathSearchOptions srch_opts,
                        std::vector<std::filesystem::path>& resolved_paths);

  typedef std::function<void(CLIParseState&, const tmedia::CLIArg arg)> ArgParseFunc;
  typedef std::map<std::string_view, ArgParseFunc, std::less<>> ArgParseMap;

  void cli_arg_help(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_version(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ffmpeg_version(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_curses_version(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_lib_versions(CLIParseState& ps, const tmedia::CLIArg arg);

  void cli_arg_background(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_chars(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_color(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_fullscreen(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_grayscale(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_repeat(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_repeat(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_repeat_one(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_mute(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_refresh_rate(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_shuffle(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_volume(CLIParseState& ps, const tmedia::CLIArg arg);

  void cli_arg_ignore_video_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_audio_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_images_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_recurse_global(CLIParseState& ps, const tmedia::CLIArg arg);

  void cli_arg_ignore_video_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_audio_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_images_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_recurse_local(CLIParseState& ps, const tmedia::CLIArg arg);


  TMediaCLIParseRes tmedia_parse_cli(int argc, char** argv) {
    CLIParseState ps;
    ps.tmss.muted = false;
    ps.tmss.refresh_rate_fps = 24;
    ps.tmss.scaling_algorithm = ScalingAlgo::BOX_SAMPLING;
    ps.tmss.loop_type = LoopType::NO_LOOP;
    ps.tmss.vom = VidOutMode::PLAIN;
    ps.tmss.volume = 1.0;
    ps.tmss.fullscreen = false;
    ps.tmss.ascii_display_chars = ASCII_STANDARD_CHAR_MAP;
    ps.tmss.shuffled = false;

    if (argc == 1) {
      std::cout << help_text << std::endl;
      return { ps.tmss, true };
    }

    std::vector<tmedia::CLIArg> parsed_cli = tmedia::cli_parse(argc, argv, "",
    {"volume", "chars", "refresh-rate"});


    static const ArgParseMap short_exiting_opt_map{
      {"h", cli_arg_help},
      {"v", cli_arg_version}
    };

    static const ArgParseMap long_exiting_opt_map{
      {"help", cli_arg_help},
      {"version", cli_arg_version},
      {"ffmpeg-version", cli_arg_ffmpeg_version},
      {"curses-version", cli_arg_curses_version},
      {"dep-version", cli_arg_lib_versions},
      {"dep-versions", cli_arg_lib_versions},
      {"lib-version", cli_arg_lib_versions},
      {"lib-versions", cli_arg_lib_versions},
    };

    static const ArgParseMap short_global_argparse_map{
      {"c", cli_arg_color},
      {"b", cli_arg_background},
      {"g", cli_arg_grayscale},
      {"m", cli_arg_mute},
      {"f", cli_arg_fullscreen},
      {"s", cli_arg_shuffle},
      {"r", cli_arg_recurse_global},
    };

    static const ArgParseMap long_global_argparse_map{
      {"no-repeat", cli_arg_no_repeat},
      {"no-loop", cli_arg_no_repeat},
      {"repeat", cli_arg_repeat},
      {"loop", cli_arg_repeat},
      {"repeat-one", cli_arg_repeat_one},
      {"loop-one", cli_arg_repeat_one},
      {"volume", cli_arg_volume},
      {"shuffle", cli_arg_shuffle},
      {"shuffled", cli_arg_shuffle},
      {"refresh-rate", cli_arg_refresh_rate},
      {"chars", cli_arg_chars},
      {"color", cli_arg_color},
      {"colour", cli_arg_color},
      {"colored", cli_arg_color},
      {"coloured", cli_arg_color},
      {"gray", cli_arg_grayscale},
      {"grey", cli_arg_grayscale},
      {"greyscale", cli_arg_grayscale},
      {"grayscale", cli_arg_grayscale},
      {"background", cli_arg_background},
      {"mute", cli_arg_mute},
      {"muted", cli_arg_mute},
      {"fullscreen", cli_arg_fullscreen},
      {"fullscreened", cli_arg_fullscreen},

      // path searching opts
      {"ignore-audio", cli_arg_ignore_audio_global},
      {"ignore-audios", cli_arg_ignore_audio_global},
      {"ignore-video", cli_arg_ignore_video_global},
      {"ignore-videos", cli_arg_ignore_video_global},
      {"ignore-image", cli_arg_ignore_images_global},
      {"ignore-images", cli_arg_ignore_images_global},
      {"recurse", cli_arg_recurse_global},
      {"recursive", cli_arg_recurse_global}
    };

    static const ArgParseMap short_local_argparse_map{
      {"r", cli_arg_recurse_local}
    };

    static const ArgParseMap long_local_argparse_map{
      {"ignore-audio", cli_arg_ignore_audio_local},
      {"ignore-audios", cli_arg_ignore_audio_local},
      {"ignore-video", cli_arg_ignore_video_local},
      {"ignore-videos", cli_arg_ignore_video_local},
      {"ignore-image", cli_arg_ignore_images_local},
      {"ignore-images", cli_arg_ignore_images_local},
      {"recurse", cli_arg_recurse_local},
      {"recursive", cli_arg_recurse_local},
    };

    for (const tmedia::CLIArg& arg : parsed_cli) {
      switch (arg.arg_type) {
        case tmedia::CLIArgType::POSITIONAL: {
          ps.paths.push_back(MediaPath(arg.value));
        } break;
        case tmedia::CLIArgType::OPTION: {
          if (arg.prefix == "-") {
            if (short_exiting_opt_map.count(arg.value)) {
              short_exiting_opt_map.at(arg.value)(ps, arg);
              return { ps.tmss, true };
            } else if (short_global_argparse_map.count(arg.value)) {
              short_global_argparse_map.at(arg.value)(ps, arg);
            } else {
              throw std::runtime_error(fmt::format("[{}] Unrecognized "
              "short option: '{}'. Use the '{} --help' command to see all "
              "available cli options", FUNCDINFO, arg.value, argv[0]));
            }
          } else if (arg.prefix == "--") {
            if (long_exiting_opt_map.count(arg.value)) {
              long_exiting_opt_map.at(arg.value)(ps, arg);
              return { ps.tmss, true };
            } else if (long_global_argparse_map.count(arg.value)) {
              long_global_argparse_map.at(arg.value)(ps, arg);
            } else {
              throw std::runtime_error(fmt::format("[{}] Unrecognized "
              "long option: '{}'. Use the '{} --help' command to see all "
              "available cli options", FUNCDINFO, arg.value, argv[0]));
            }
          } else if (arg.prefix == ":") {
            if (short_local_argparse_map.count(arg.value)) {
              short_local_argparse_map.at(arg.value)(ps, arg);
            } else if (long_local_argparse_map.count(arg.value)) {
              long_local_argparse_map.at(arg.value)(ps, arg);
            } else {
              throw std::runtime_error(fmt::format("[{}] Unrecognized "
              "local option: '{}'. Use the '{} --help' command to see all "
              "available cli options", FUNCDINFO, arg.value, argv[0]));
            }
          }
        } break;
      }
    }

    if (ps.paths.size() == 0UL) {
      throw std::runtime_error(fmt::format("[{}] No paths entered", FUNCDINFO));
    }

    for (const MediaPath& path : ps.paths) {
      resolve_cli_path(path.path,
            resolve_path_search_options(ps.srch_opts, path.srch_opts),
            ps.tmss.media_files);
    }

    if (ps.tmss.media_files.size() == 0UL) {
      throw std::runtime_error(fmt::format("[{}] No media files found.", FUNCDINFO));
    }

    if (ps.colored)
      ps.tmss.vom = ps.background ? VidOutMode::COLOR_BG : VidOutMode::COLOR;
    else if (ps.grayscale)
      ps.tmss.vom = ps.background ? VidOutMode::GRAY_BG : VidOutMode::GRAY;

    return TMediaCLIParseRes(ps.tmss, false);
  }

  void cli_arg_help(CLIParseState& ps, const tmedia::CLIArg arg) {
    std::cout << help_text << std::endl;
    (void)ps; (void)arg;
  }

  void cli_arg_version(CLIParseState& ps, const tmedia::CLIArg arg) {
    std::cout << TMEDIA_VERSION << std::endl;
    (void)ps; (void)arg;
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

  void cli_arg_lib_versions(CLIParseState& ps, const tmedia::CLIArg arg) {
    print_curses_version();
    print_ffmpeg_version();
    (void)ps; (void)arg;
  }

  void cli_arg_background(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.background = true;
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

  void cli_arg_fullscreen(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.fullscreen = true;
    (void)arg;
  }

  void cli_arg_grayscale(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.grayscale = true;
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

  void cli_arg_refresh_rate(CLIParseState& ps, const tmedia::CLIArg arg) {
    int res = 0;
    
    try {
      res = strtoi32(arg.param);
    } catch (const std::runtime_error& err) {
      throw std::runtime_error(fmt::format("[{}] Could not parse param {} as "
      "integer: \n\t{}", FUNCDINFO, arg.param, err.what()));
    }

    if (res <= 0) {
      throw std::runtime_error(fmt::format("[{}] refresh rate must be greater "
      "than 0. (got {})", FUNCDINFO, res));
    }

    ps.tmss.refresh_rate_fps = res;
  }

  void cli_arg_shuffle(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.tmss.shuffled = true;
    (void)arg;
  }

  void cli_arg_volume(CLIParseState& ps, const tmedia::CLIArg arg) {
    double volume = 1.0;
    try {
      volume = parse_percentage(arg.param);
    } catch (const std::runtime_error& err) {
      throw std::runtime_error(fmt::format("[{}] Could not parse parameter {} "
      "as a percentage value: \n\t{}", FUNCDINFO, arg.param, err.what()));
    }

    if (volume < 0.0 || volume > 1.0) {
      throw std::runtime_error(fmt::format("[{}] Volume out of bounds "
      " [0.0, 1.0] (got {})", FUNCDINFO, volume));
    }

    ps.tmss.volume = volume;
  }

  void cli_arg_ignore_video_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.ignore_video = true;
    (void)arg;
  }

  void cli_arg_ignore_audio_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.ignore_audio = true;
    (void)arg;
  }

  void cli_arg_ignore_images_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.ignore_images = true;
    (void)arg;
  }

  void cli_arg_recurse_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.recurse = true;
    (void)arg;
  }

  void cli_arg_ignore_video_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_video = InheritableBoolean::TRUE;
    }
    (void)arg;
  }

  void cli_arg_ignore_audio_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_audio = InheritableBoolean::TRUE;
    }
    (void)arg;
  }

  void cli_arg_ignore_images_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_images = InheritableBoolean::TRUE;
    }
    (void)arg;
  }

  void cli_arg_recurse_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.recurse = InheritableBoolean::TRUE;
    }
    (void)arg;
  }

  bool resolve_inheritable_bool(InheritableBoolean ib, bool parent_bool) {
    return ib == InheritableBoolean::INHERIT ? parent_bool : ( ib == InheritableBoolean::TRUE ? true : false);
  }

  MediaPathSearchOptions resolve_path_search_options(MediaPathSearchOptions global, MediaPathLocalSearchOptions local) {
    MediaPathSearchOptions res;
    res.ignore_video = resolve_inheritable_bool(local.ignore_video, global.ignore_video);
    res.ignore_audio = resolve_inheritable_bool(local.ignore_audio, global.ignore_audio);
    res.ignore_images = resolve_inheritable_bool(local.ignore_images, global.ignore_images);
    res.recurse = resolve_inheritable_bool(local.recurse, global.recurse);
    return res;
  }

  bool test_media_file(const std::filesystem::path& path, MediaPathSearchOptions search_opts) {
    std::optional<MediaType> media_type = media_type_probe(path);
    if (!media_type.has_value()) return false;

    switch (*media_type) {
      case MediaType::VIDEO: return !search_opts.ignore_video;
      case MediaType::AUDIO: return !search_opts.ignore_audio;
      case MediaType::IMAGE: return !search_opts.ignore_images;
    }

    return false;
  }


  void resolve_cli_path(const std::filesystem::path& path, MediaPathSearchOptions srch_opts, std::vector<std::filesystem::path>& resolved_paths) {
    std::stack<std::filesystem::path> to_search;
    to_search.push(path);
    std::error_code ec;

    while (!to_search.empty()) {
      const std::filesystem::path curr = std::move(to_search.top());
      to_search.pop();

      if (!std::filesystem::exists(curr, ec)) {
        throw std::runtime_error(fmt::format("[{}] Cannot open nonexistent "
        "path: {}", FUNCDINFO, path.c_str()));
      }

      if (std::filesystem::is_directory(curr, ec)) {
        std::vector<std::string> media_file_paths;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(curr)) {
          if (std::filesystem::is_directory(entry.path(), ec) && srch_opts.recurse) {
            to_search.push(std::move(entry.path()));
          } else if (std::filesystem::is_regular_file(entry.path()) && test_media_file(entry.path(), srch_opts)) {
            media_file_paths.push_back(std::move(entry.path().string()));
          }
        }

        SI::natural::sort(media_file_paths);

        for (auto&& media_file_path : media_file_paths) {
          resolved_paths.push_back(std::move(media_file_path));
        }
      } else if (std::filesystem::is_regular_file(curr) && test_media_file(curr, srch_opts)) {
        resolved_paths.push_back(curr);
      }
    }
  }


// };