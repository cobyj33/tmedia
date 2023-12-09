#include "tmedia.h"

#include "cli_iter.h"
#include "playlist.h"
#include "formatting.h"
#include "version.h"
#include "ascii.h"

#include <natural_sort.hpp>

#include <vector>
#include <cstddef>
#include <map>
#include <functional>
#include <filesystem>
#include <iostream>
#include <charconv>
#include <stack>

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

  const char* help_text = "Usage: tmedia [--help] [--version] [--color] [--grayscale] [--background] [--dump] [--muted] [--fullscreen] [--volume VAR] [--loop VAR] [--max-fps VAR] [--scaling-algo VAR] [--shuffle] [--chars VAR] [--recursive] [--ffmpeg-version] [--curses-version] paths\n"
  "\n"
  "----------------------------CONTROLS--------------------------------------\n"
  "  Video and Audio Controls\n"
  "    - Space                       Play and Pause\n"
  "    - Up Arrow                    Increase Volume 1%\n"
  "    - Down Arrow                  Decrease Volume 1%\n"
  "    - Left Arrow                  Skip Backward 5 Seconds\n"
  "    - Right Arrow                 Skip Forward 5 Seconds\n"
  "    - Escape, Backspace, or 'q'   Quit Program\n"
  "    - '0'                         Restart Playback\n"
  "    - '1' through '9'             Skip To n/10 of the Media's Duration\n"
  "    - 'L'                         Switch looping type of playback\n"
  "    - 'M'                         Mute/Unmute Audio\n"
  "  Video, Audio, and Image Controls\n"
  "    - 'C'    Display Color (on supported terminals)\n"
  "    - 'G'    Display Grayscale (on supported terminals)\n"
  "    - 'B'    Display Spaces (on supported terms & while colored/grayscale)"
  "    - 'N'    Skip to Next Media File\n"
  "    - 'P'    Rewind to Previous Media File\n"
  "    - 'R'    Fully Refresh the Screen\n"
  "\n"
  "\n"
  "  Positional arguments:\n"
  "    paths                    The the paths to files or directories to be played.\n"
  "                             Multiple files will be played one after the other.\n"
  "                             Directories will be expanded so any media files inside them will be played. [nargs: 0 or more] \n"
  "\n"
  "  Optional arguments:\n"
  "    Help and Versioning: \n"
  "    -h, --help                 shows help message and exits \n"
  "    -v, --version              prints version information and exits \n"
  "    --ffmpeg-version           Print the version of linked FFmpeg libraries \n"
  "    --curses-version           Print the version of linked Curses libraries\n"

  "  Video Output: \n"
  "    -c, --color                Play the video with color \n"
  "    -g, --gray, --grayscale    Play the video in grayscale \n"
  "    -b, --background           Do not show characters, only the background \n"
  "    -f, --fullscreen           Begin the player in fullscreen mode \n"
  "    --refresh-rate             Set the refresh rate of tmedia\n"
  "    --chars                    The characters from darkest to lightest to use as display \n"

  "  Audio Output: \n"
  "    --volume                   Set initial volume (must be between 0.0 and 1.0) \n"
  "    -m, --mute, --muted        Mute the audio playback \n"

  "  Playlist Controls: \n"
  "    --loop-type                Set the loop type of the player ('no-loop', 'repeat', or 'repeat-one') \n"
  "    -s, --shuffle, --shuffled  Shuffle the given playlist \n"

  "  File Searching: \n"
  "    -r, --recursive            Recurse through directories recursively to find media files \n"
  "    --ignore-images            Ignore image files while searching through directories\n"
  "    --ignore-video             Ignore video files while searching through directories\n"
  "    --ignore-audio             Ignore audio files while searching through directories\n"
  "    :r, :recursive             Recurse through directories of the last listed path to find media files \n"
  "    :ignore-images             Ignore image files while searching through the last listed path\n"
  "    :ignore-video              Ignore video files while searching through the last listed path\n"
  "    :ignore-audio              Ignore audio files while searching through the last listed path\n";

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
    std::string path;
    MediaPathLocalSearchOptions search_options;
    MediaPath(std::string path) : path(path) {} 
  };

  struct TMediaCLIParseState {
    TMediaStartupState tmss;
    std::vector<MediaPath> paths;

    MediaPathSearchOptions search_options;

    bool colored = false;
    bool grayscale = false;
    bool background = false;
  };

  std::vector<std::string> resolve_cli_paths(const std::vector<MediaPath>& paths, MediaPathSearchOptions global_search_opts);

  typedef std::function<void(TMediaCLIParseState&, tmedia::CLIArg& arg)> TMediaCLIArgParseFunc;
  typedef std::map<std::string, TMediaCLIArgParseFunc> TMediaCliArgParseMap;

  void tmedia_cli_arg_help(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_version(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_ffmpeg_version(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_curses_version(TMediaCLIParseState& ps, tmedia::CLIArg& arg);

  void tmedia_cli_arg_background(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_chars(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_color(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_fullscreen(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_grayscale(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_loop_type(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_mute(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_refresh_rate(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_shuffle(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_volume(TMediaCLIParseState& ps, tmedia::CLIArg& arg);

  void tmedia_cli_arg_ignore_video_global(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_ignore_audio_global(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_ignore_images_global(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_recurse_global(TMediaCLIParseState& ps, tmedia::CLIArg& arg);

  void tmedia_cli_arg_ignore_video_local(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_ignore_audio_local(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_ignore_images_local(TMediaCLIParseState& ps, tmedia::CLIArg& arg);
  void tmedia_cli_arg_recurse_local(TMediaCLIParseState& ps, tmedia::CLIArg& arg);


  TMediaCLIParseRes tmedia_parse_cli(int argc, char** argv) {
    TMediaCLIParseState ps;
    ps.tmss.muted = false;
    ps.tmss.refresh_rate_fps = 24;
    ps.tmss.scaling_algorithm = ScalingAlgo::BOX_SAMPLING;
    ps.tmss.loop_type = LoopType::NO_LOOP;
    ps.tmss.vom = VideoOutputMode::TEXT_ONLY;
    ps.tmss.volume = 1.0;
    ps.tmss.fullscreen = false;
    ps.tmss.ascii_display_chars = ASCII_STANDARD_CHAR_MAP;
    ps.tmss.shuffled = false;

    if (argc == 1) {
      std::cout << help_text << std::endl;
      return { ps.tmss, true };
    }

    std::vector<tmedia::CLIArg> parsed_cli = tmedia::cli_parse(argc, argv, "l",
    {"scaling-algo", "volume", "loop-type", "chars", "refresh-rate"});

    TMediaCliArgParseMap exiting_opt_map{
      {"-h", tmedia_cli_arg_help},
      {"-v", tmedia_cli_arg_version},
      {"--help", tmedia_cli_arg_help},
      {"--version", tmedia_cli_arg_version},
      {"--ffmpeg-version", tmedia_cli_arg_ffmpeg_version},
      {"--curses-version", tmedia_cli_arg_curses_version},
    };

    TMediaCliArgParseMap argparse_map{
      {"-c", tmedia_cli_arg_color},
      {"-b", tmedia_cli_arg_background},
      {"-g", tmedia_cli_arg_grayscale},
      {"-m", tmedia_cli_arg_mute},
      {"-f", tmedia_cli_arg_fullscreen},
      {"-s", tmedia_cli_arg_shuffle},
      {"-r", tmedia_cli_arg_recurse_global},
      {"--loop-type", tmedia_cli_arg_loop_type},
      {"--volume", tmedia_cli_arg_volume},
      {"--shuffle", tmedia_cli_arg_shuffle},
      {"--shuffled", tmedia_cli_arg_shuffle},
      {"--refresh-rate", tmedia_cli_arg_refresh_rate},
      {"--chars", tmedia_cli_arg_chars},
      {"--color", tmedia_cli_arg_color},
      {"--colored", tmedia_cli_arg_color},
      {"--gray", tmedia_cli_arg_grayscale},
      {"--grey", tmedia_cli_arg_grayscale},
      {"--greyscale", tmedia_cli_arg_grayscale},
      {"--grayscale", tmedia_cli_arg_grayscale},
      {"--background", tmedia_cli_arg_background},
      {"--mute", tmedia_cli_arg_mute},
      {"--muted", tmedia_cli_arg_mute},
      {"--fullscreen", tmedia_cli_arg_fullscreen},

      // path searching opts
      {"--ignore-audio", tmedia_cli_arg_ignore_audio_global},
      {"--ignore-audios", tmedia_cli_arg_ignore_audio_global},
      {"--ignore-video", tmedia_cli_arg_ignore_video_global},
      {"--ignore-videos", tmedia_cli_arg_ignore_video_global},
      {"--ignore-image", tmedia_cli_arg_ignore_images_global},
      {"--ignore-images", tmedia_cli_arg_ignore_images_global},
      {"--recurse", tmedia_cli_arg_recurse_global},
      {"--recursive", tmedia_cli_arg_recurse_global},

      {":r", tmedia_cli_arg_recurse_local},
      {":ignore-audio", tmedia_cli_arg_ignore_audio_local},
      {":ignore-audios", tmedia_cli_arg_ignore_audio_local},
      {":ignore-video", tmedia_cli_arg_ignore_video_local},
      {":ignore-videos", tmedia_cli_arg_ignore_video_local},
      {":ignore-image", tmedia_cli_arg_ignore_images_local},
      {":ignore-images", tmedia_cli_arg_ignore_images_local},
      {":recurse", tmedia_cli_arg_recurse_local},
      {":recursive", tmedia_cli_arg_recurse_local},
    };

    for (std::size_t i = 0; i < parsed_cli.size(); i++) {
      switch (parsed_cli[i].arg_type) {
        case tmedia::CLIArgType::POSITIONAL: {
          ps.paths.push_back(MediaPath(parsed_cli[i].value));
        } break;
        case tmedia::CLIArgType::OPTION: {
          const std::string fullopt = parsed_cli[i].prefix + parsed_cli[i].value;
          if (exiting_opt_map.count(fullopt) == 1) {
            exiting_opt_map[fullopt](ps, parsed_cli[i]);
            return { ps.tmss, true };
          } else if (argparse_map.count(fullopt) == 1) {
            argparse_map[fullopt](ps, parsed_cli[i]);
          } else {
            throw std::runtime_error("[tmedia_parse_cli] Unrecognized option: " + fullopt);
          }
        } break;
      }
    }

    if (ps.paths.size() == 0UL) {
      throw std::runtime_error("[tmedia_parse_cli] No paths entered");
    }

    ps.tmss.media_files = resolve_cli_paths(ps.paths, ps.search_options);
    if (ps.tmss.media_files.size() == 0UL) {
      throw std::runtime_error("[tmedia_parse_cli] No media files found.");
    }

    if (ps.colored)
      ps.tmss.vom = ps.background ? VideoOutputMode::COLORED_BACKGROUND_ONLY : VideoOutputMode::COLORED;
    else if (ps.grayscale)
      ps.tmss.vom = ps.background ? VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY : VideoOutputMode::GRAYSCALE;

    return TMediaCLIParseRes(ps.tmss, false);
  }

  void tmedia_cli_arg_help(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    std::cout << help_text << std::endl;
    (void)ps; (void)arg;
  }

  void tmedia_cli_arg_version(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    std::cout << TMEDIA_VERSION << std::endl;
    (void)ps; (void)arg;
  }

  void tmedia_cli_arg_ffmpeg_version(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    std::cout << "libavformat: " << LIBAVFORMAT_VERSION_MAJOR << ":" << LIBAVFORMAT_VERSION_MINOR << ":" << LIBAVFORMAT_VERSION_MICRO << std::endl;
    std::cout << "libavcodec: " << LIBAVCODEC_VERSION_MAJOR << ":" << LIBAVCODEC_VERSION_MINOR << ":" << LIBAVCODEC_VERSION_MICRO << std::endl;
    std::cout << "libavutil: " << LIBAVUTIL_VERSION_MAJOR << ":" << LIBAVUTIL_VERSION_MINOR << ":" << LIBAVUTIL_VERSION_MICRO << std::endl;
    std::cout << "libswresample: " << LIBSWRESAMPLE_VERSION_MAJOR << ":" << LIBSWRESAMPLE_VERSION_MINOR << ":" << LIBSWRESAMPLE_VERSION_MICRO << std::endl;
    std::cout << "libswscale: " << LIBSWSCALE_VERSION_MAJOR << ":" << LIBSWSCALE_VERSION_MINOR << ":" << LIBSWSCALE_VERSION_MICRO << std::endl;
    (void)ps; (void)arg;
  }

  void tmedia_cli_arg_curses_version(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    std::cout << curses_version() << std::endl;
    (void)ps; (void)arg;
  }

  void tmedia_cli_arg_background(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.background = true;
    (void)arg;
  }

  void tmedia_cli_arg_chars(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.tmss.ascii_display_chars = arg.param;
    (void)arg;
  }

  void tmedia_cli_arg_color(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.colored = true;
    (void)arg;
  }

  void tmedia_cli_arg_fullscreen(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.tmss.fullscreen = true;
    (void)arg;
  }

  void tmedia_cli_arg_grayscale(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.grayscale = true;
    (void)arg;
  }

  void tmedia_cli_arg_loop_type(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.tmss.loop_type = loop_type_from_str(arg.param);
  }

  void tmedia_cli_arg_mute(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.tmss.muted = true;
    (void)arg;
  }

  void tmedia_cli_arg_refresh_rate(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    int res = 0;
    
    try {
      res = strtoi32(arg.param);
    } catch (const std::runtime_error& err) {
      throw std::runtime_error("[tmedia_cli_arg_refresh_rate] Could not parse param " + arg.param + " as integer: " + err.what());
    }

    if (res <= 0) {
      throw std::runtime_error("[tmedia_cli_arg_refresh_rate] refresh rate must be greater than 0. (got " + std::to_string(res) + ")");
    }

    ps.tmss.refresh_rate_fps = res;
  }

  void tmedia_cli_arg_shuffle(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.tmss.shuffled = true;
    (void)arg;
  }

  void tmedia_cli_arg_volume(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    double volume = 1.0;
    try {
      volume = parse_percentage(arg.param);
    } catch (const std::runtime_error& err) {
      throw std::runtime_error("[tmedia_cli_arg_volume] Could not parse parameter " + arg.param + " as a percentage value: " + err.what());
    }

    if (volume < 0.0 || volume > 1.0) {
      throw std::runtime_error("[tmedia_cli_arg_volume] Volume out of bounds 0.0 to 1.0");
    }

    ps.tmss.volume = volume;
  }

  void tmedia_cli_arg_ignore_video_global(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.search_options.ignore_video = true;
    (void)arg;
  }

  void tmedia_cli_arg_ignore_audio_global(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.search_options.ignore_audio = true;
    (void)arg;
  }

  void tmedia_cli_arg_ignore_images_global(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.search_options.ignore_images = true;
    (void)arg;
  }

  void tmedia_cli_arg_recurse_global(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    ps.search_options.recurse = true;
    (void)arg;
  }

  void tmedia_cli_arg_ignore_video_local(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].search_options.ignore_video = InheritableBoolean::TRUE;
    }
    (void)arg;
  }

  void tmedia_cli_arg_ignore_audio_local(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].search_options.ignore_audio = InheritableBoolean::TRUE;
    }
    (void)arg;
  }

  void tmedia_cli_arg_ignore_images_local(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].search_options.ignore_images = InheritableBoolean::TRUE;
    }
    (void)arg;
  }

  void tmedia_cli_arg_recurse_local(TMediaCLIParseState& ps, tmedia::CLIArg& arg) {
    if (ps.paths.size() > 0UL) {
      ps.paths[ps.paths.size() - 1UL].search_options.recurse = InheritableBoolean::TRUE;
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

  bool test_media_file(std::filesystem::path path, MediaPathSearchOptions search_opts) {
    std::optional<MediaType> media_type = media_type_guess(path.string());
    if (!media_type.has_value()) return false;
    std::pair<MediaType, bool> allowed_media_types[] = {
      {MediaType::VIDEO, !search_opts.ignore_video},
      {MediaType::AUDIO, !search_opts.ignore_audio},
      {MediaType::IMAGE, !search_opts.ignore_images},
    };

    for (const std::pair<MediaType, bool>& val : allowed_media_types) {
      if (val.first == *media_type) return val.second;
    }
    
    return false;
  }


  std::vector<std::string> resolve_cli_path(std::string path_str, MediaPathSearchOptions search_options) {
    std::vector<std::string> resolved_paths;

    if (path_str.length() == 0) {
      throw std::runtime_error("[resolve_cli_path] Cannot open empty path");
    }

    std::stack<std::filesystem::path> to_search;
    to_search.push(std::filesystem::path(path_str));
    std::error_code ec;

    while (!to_search.empty()) {
      const std::filesystem::path path = to_search.top();
      to_search.pop();

      if (!std::filesystem::exists(path, ec)) {
        throw std::runtime_error("[resolve_cli_path] Cannot open nonexistent path: " + path_str);
      }

      if (std::filesystem::is_directory(path, ec)) {
        std::vector<std::string> media_file_paths;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path)) {
          if (std::filesystem::is_directory(entry.path(), ec) && search_options.recurse) {
            to_search.push(entry.path());
          } else if (std::filesystem::is_regular_file(entry.path()) && test_media_file(entry.path(), search_options)) {
            media_file_paths.push_back(entry.path().string());
          }
        }

        SI::natural::sort(media_file_paths);

        for (const std::string& media_file_path : media_file_paths) {
          resolved_paths.push_back(media_file_path);
        }
      } else if (std::filesystem::is_regular_file(path) && test_media_file(path, search_options)) {
        resolved_paths.push_back(path_str);
      }

    }

    return resolved_paths;
  }

  std::vector<std::string> resolve_cli_paths(const std::vector<MediaPath>& paths, MediaPathSearchOptions search_opts) {
    std::vector<std::string> valid_paths;

    for (std::size_t i = 0; i < paths.size(); i++) {
      std::vector<std::string> resolved = resolve_cli_path(paths[i].path, resolve_path_search_options(search_opts, paths[i].search_options));
      for (std::size_t i = 0; i < resolved.size(); i++) {
        valid_paths.push_back(resolved[i]);
      }
    }

    return valid_paths;
  }


// };