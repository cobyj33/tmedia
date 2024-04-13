#include <tmedia/tmedia.h>

#include <tmedia/cli/cli_iter.h>
#include <tmedia/media/playlist.h>
#include <tmedia/util/formatting.h>
#include <tmedia/version.h>
#include <tmedia/util/funcmac.h>
#include <tmedia/image/scale.h>
#include <tmedia/ffmpeg/probe.h>
#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/util/funcmac.h>

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

namespace fs = std::filesystem;

#undef TRUE
#undef FALSE

// namespace tmedia {
const char* TMEDIA_CLI_OPTIONS_DESC = ""
  "-------------------------------CLI ARGUMENTS------------------------------\n"
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
  "    NOTE: all local (:) options override global options\n"
  "\n"
  "    -r, --recursive        Recurse child directories to find media files \n"
  "    --ignore-images        Ignore image files while searching directories\n"
  "    --ignore-video         Ignore video files while searching directories\n"
  "    --ignore-audio         Ignore audio files while searching directories\n"
  "    --probe, --no-probe    (Don't) probe all files\n"
  "    :r, :recursive         Recurse child directories of the last listed path \n"
  "    :ignore-images         Ignore image files when searching last listed path\n"
  "    :ignore-video          Ignore video files when searching last listed path\n"
  "    :ignore-audio          Ignore audio files when searching last listed path\n"
  "    :probe, :no-probe      (Don't) probe last file/directory\n"
  "---------------------------------------------------------------------------";

  /**
   * The idea of inheritable boolean is that each media item will either have
   * global or local values assigned to it (such as configuring to ignore images
   * or videos). Therefore, when the inheritable boolean is INHERIT, we signal
   * to read the global value rather than any local value.
  */
  enum class InheritBool {
    FALSE = 0,
    TRUE = 1,
    INHERIT = -1
  };

  struct MediaPathSearchOptions {
    bool ignore_audio = false;
    bool ignore_video = false;
    bool ignore_images = false;
    bool recurse = false;
    bool probe = false;
  };

  struct MediaPathLocalSearchOptions {
    InheritBool ignore_audio = InheritBool::INHERIT;
    InheritBool ignore_video = InheritBool::INHERIT;
    InheritBool ignore_images = InheritBool::INHERIT;
    InheritBool recurse = InheritBool::INHERIT;
    InheritBool probe = InheritBool::INHERIT;
  };

  bool resolve_inheritable_bool(InheritBool ib, bool parent_bool);
  MediaPathSearchOptions resolve_path_search_options(MediaPathSearchOptions global, MediaPathLocalSearchOptions local);

  struct MediaPath {
    fs::path path;
    MediaPathLocalSearchOptions srch_opts;
    MediaPath(std::string_view path) : path(std::move(fs::path(path))) {} 
    MediaPath(const fs::path& path) : path(path) {} 
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
                        std::vector<fs::path>& resolved_paths);

  typedef std::function<void(CLIParseState&, const tmedia::CLIArg arg)> ArgParseFunc;
  typedef std::map<std::string_view, ArgParseFunc, std::less<>> ArgParseMap;

  void print_help_text();

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
  void cli_arg_probe_global(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_probe_global(CLIParseState& ps, const tmedia::CLIArg arg);

  void cli_arg_ignore_video_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_audio_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_ignore_images_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_recurse_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_probe_local(CLIParseState& ps, const tmedia::CLIArg arg);
  void cli_arg_no_probe_local(CLIParseState& ps, const tmedia::CLIArg arg);


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
      print_help_text();
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
      {"recursive", cli_arg_recurse_global},
      {"probe", cli_arg_probe_global},
      {"no-probe", cli_arg_no_probe_global},
      {"dont-probe", cli_arg_no_probe_global},
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
      {"probe", cli_arg_probe_local},
      {"no-probe", cli_arg_no_probe_local},
      {"dont-probe", cli_arg_no_probe_local},
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
      ps.argerrs.push_back(fmt::format("[{}] No paths entered", FUNCDINFO));
    }

    if (ps.argerrs.size() > 0) {
      std::stringstream ss;
      for (std::size_t i = 0; i < ps.argerrs.size(); i++) {
        ss << ps.argerrs[i];
        if (i < ps.argerrs.size() - 1) ss << '\n'; 
      }
      throw std::runtime_error(ss.str());
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

  void print_help_text() {
    std::cout << "Usage: tmedia [OPTIONS] paths\n" << std::endl;
    std::cout << TMEDIA_CONTROLS_USAGE << '\n' << std::endl;
    std::cout << TMEDIA_CLI_OPTIONS_DESC << std::endl;
  }

  void cli_arg_help(CLIParseState& ps, const tmedia::CLIArg arg) {
    print_help_text();
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

  void cli_arg_probe_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.probe = true;
    (void)arg;
  }

  void cli_arg_no_probe_global(CLIParseState& ps, const tmedia::CLIArg arg) {
    ps.srch_opts.probe = false;
    (void)arg;
  }

  void cli_arg_ignore_video_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_video = InheritBool::TRUE;
    }
    (void)arg;
  }

  void cli_arg_ignore_audio_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_audio = InheritBool::TRUE;
    }
    (void)arg;
  }

  void cli_arg_ignore_images_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.ignore_images = InheritBool::TRUE;
    }
    (void)arg;
  }

  void cli_arg_recurse_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.recurse = InheritBool::TRUE;
    }
    (void)arg;
  }

  void cli_arg_probe_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.probe = InheritBool::TRUE;
    }
    (void)arg;
  }

  void cli_arg_no_probe_local(CLIParseState& ps, const tmedia::CLIArg arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].srch_opts.probe = InheritBool::FALSE;
    }
    (void)arg;
  }

  bool resolve_inheritable_bool(InheritBool ib, bool parent_bool) {
    return ib == InheritBool::INHERIT ? parent_bool : (ib == InheritBool::TRUE);
  }

  MediaPathSearchOptions resolve_path_search_options(MediaPathSearchOptions global, MediaPathLocalSearchOptions local) {
    MediaPathSearchOptions res;
    res.ignore_video = resolve_inheritable_bool(local.ignore_video, global.ignore_video);
    res.ignore_audio = resolve_inheritable_bool(local.ignore_audio, global.ignore_audio);
    res.ignore_images = resolve_inheritable_bool(local.ignore_images, global.ignore_images);
    res.recurse = resolve_inheritable_bool(local.recurse, global.recurse);
    res.probe = resolve_inheritable_bool(local.probe, global.probe);
    return res;
  }

  bool test_media_file_probe(const fs::path& path, MediaPathSearchOptions search_opts) {
    std::optional<MediaType> media_type = media_type_probe(path);
    if (!media_type.has_value()) return false;

    switch (*media_type) {
      case MediaType::VIDEO: return !search_opts.ignore_video;
      case MediaType::AUDIO: return !search_opts.ignore_audio;
      case MediaType::IMAGE: return !search_opts.ignore_images;
    }

    return false;
  }

  bool test_media_file_stupid(const fs::path& path, MediaPathSearchOptions search_opts) {
    std::optional<MediaType> media_type = media_type_from_path(path);
    if (!media_type.has_value()) return false;
    
    if (media_type) {
      switch (*media_type) {
        case MediaType::VIDEO: return !search_opts.ignore_video;
        case MediaType::AUDIO: return !search_opts.ignore_audio;
        case MediaType::IMAGE: return !search_opts.ignore_images;
      }
    }

    return false;
  }

  bool test_media_file(const fs::path& path, MediaPathSearchOptions search_opts) {
    return search_opts.probe ? test_media_file_probe(path, search_opts)
                             : test_media_file_stupid(path, search_opts);
  }


  void resolve_cli_path(const fs::path& path, MediaPathSearchOptions srch_opts, std::vector<fs::path>& resolved_paths) {
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
        for (const fs::directory_entry& entry : fs::directory_iterator(curr)) {
          if (fs::is_directory(entry.path(), ec) && srch_opts.recurse) {
            to_search.push(std::move(entry.path()));
          } else if (fs::is_regular_file(entry.path()) && test_media_file(entry.path(), srch_opts)) {
            media_file_paths.push_back(std::move(entry.path().string()));
          }
        }

        SI::natural::sort(media_file_paths);

        for (auto&& media_file_path : media_file_paths) {
          resolved_paths.push_back(std::move(media_file_path));
        }
      } else if (fs::is_regular_file(curr) && test_media_file(curr, srch_opts)) {
        resolved_paths.push_back(curr);
      }
    }
  }


// };