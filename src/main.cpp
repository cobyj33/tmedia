#include "signalstate.h"

#include "wtime.h"
#include "mediafetcher.h"
#include "formatting.h"
#include "version.h"
#include "avguard.h"
#include "tmcurses.h"
#include "playlist.h"
#include "tmcurses.h"
#include "ascii.h"
#include "wminiaudio.h"
#include "sleep.h"
#include "wmath.h"
#include "boiler.h"
#include "tmedia_string.h"
#include "miscutil.h"
#include "tmedia.h"
#include "tmedia_vom.h"

#include <cstdlib>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <csignal>
#include <map>
#include <algorithm>

#include <argparse/argparse.hpp>
#include <natural_sort.hpp>

extern "C" {
#include "curses.h"
#include "miniaudio.h"
#include <libavutil/log.h>
#include <libavformat/version.h>
#include <libavutil/version.h>
#include <libavcodec/version.h>
#include <libswresample/version.h>
#include <libswscale/version.h>
}

int program_print_ffmpeg_version();
int program_print_curses_version();
int program_dump_metadata(const std::vector<std::string>& files);

bool INTERRUPT_RECEIVED = false; //defined as extern in signalstate.h
void interrupt_handler(int) {
  INTERRUPT_RECEIVED = true;
}

void on_terminate() {
  if (ncurses_is_initialized()) {
    ncurses_uninit();
  }

  std::exception_ptr exptr = std::current_exception();
  try {
    std::rethrow_exception(exptr);
  }
  catch (std::exception const& ex) {
    std::cerr << "[tmedia] Terminated due to exception: " << ex.what() << std::endl;
  }

  std::abort();
}

template <typename T>
std::string format_arg_map(std::map<std::string, T> map, std::string conjunction) {
  return format_list(transform_vec(get_strmap_keys(map), [](std::string str) { return "'" + str + "'"; }), conjunction);
}

int main(int argc, char** argv)
{
  #if USE_AV_REGISTER_ALL
  av_register_all();
  #endif
  #if USE_AVCODEC_REGISTER_ALL
  avcodec_register_all();
  #endif
  
  srand(time(nullptr));
  av_log_set_level(AV_LOG_QUIET);
  std::set_terminate(on_terminate);

  std::signal(SIGINT, interrupt_handler);
	std::signal(SIGTERM, interrupt_handler);
	std::signal(SIGABRT, interrupt_handler);

  const std::map<std::string, LoopType> VALID_LOOP_TYPE_ARGS{
      {"none", LoopType::NO_LOOP},
      {"repeat", LoopType::REPEAT},
      {"repeat-one", LoopType::REPEAT_ONE}
  };

  const std::map<std::string, ScalingAlgo> VALID_SCALING_ALGO_ARGS{
    {"nearest-neighbor", ScalingAlgo::NEAREST_NEIGHBOR},
    {"box-sampling", ScalingAlgo::BOX_SAMPLING}
  };
  
  argparse::ArgumentParser parser("tmedia", TMEDIA_VERSION);

  parser.add_argument("paths")
    .help("The the paths to files or directories to be played.\n"
    "Multiple files will be played one after the other.\n"
    "Directories will be expanded so any media files inside them will be played.")
    .nargs(argparse::nargs_pattern::any);

  parser.add_description(TMEDIA_CONTROLS_USAGE);

  parser.add_argument("-c", "--color")
    .default_value(false)
    .implicit_value(true)
    .help("Play the video with color");

  parser.add_argument("-g", "--grayscale", "--gray")
    .default_value(false)
    .implicit_value(true)
    .help("Play the video in grayscale");

  parser.add_argument("-b", "--background")
    .default_value(false)
    .implicit_value(true)
    .help("Do not show characters, only the background");

  parser.add_argument("-d", "--dump")
    .help("Print metadata about the files")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("-m", "--mute", "--muted")
    .help("Mute the audio playback")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("-f", "--fullscreen")
    .help("Begin the player in fullscreen mode")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("--volume")
    .help("Set initial volume (must be between 0.0 and 1.0)")
    .scan<'g', double>();

  parser.add_argument("-l", "--loop")
    .help("Set the loop type of the player (" + format_arg_map(VALID_LOOP_TYPE_ARGS, "or") + ")");

  parser.add_argument("--max-fps")
    .help("Set the maximum rendering fps of tmedia")
    .scan<'i', int>();
  
  parser.add_argument("--scaling-algo")
    .help("Set the scaling algorithm to use when rendering frames  (" + format_arg_map(VALID_SCALING_ALGO_ARGS, "or") + ")");

  parser.add_argument("-s", "--shuffle")
    .help("Shuffle the given playlist")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("--chars")
    .help("The characters from darkest to lightest to use as display");

  parser.add_argument("--ffmpeg-version")
    .help("Print the version of linked FFmpeg libraries")
    .default_value(false)
    .implicit_value(true);


  parser.add_argument("--curses-version")
    .help("Print the version of linked Curses libraries")
    .default_value(false)
    .implicit_value(true);

  try {
    parser.parse_args(argc, argv);
  }
  catch (const std::exception& err) {
    std::cerr << "[tmedia]: Error while parsing arguments. Call tmedia with '--help' or with no arguments to see help message" << std::endl;
    std::cerr << std::endl;
    std::cerr << err.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (parser.get<bool>("--ffmpeg-version")) return program_print_ffmpeg_version();
  if (parser.get<bool>("--curses-version")) return program_print_curses_version();
  std::vector<std::string> paths = parser.get<std::vector<std::string>>("paths");

  if (argc == 1) {
    std::cout << parser << std::endl;
    return EXIT_SUCCESS;
  }

  if (paths.size() == 0) {
    std::cerr << "[tmedia]: at least 1 path expected. 0 found. Path can be to directory or media file" << std::endl;
    return EXIT_FAILURE;
  }

  TMediaProgramData tmpd;
  tmpd.muted = false;
  tmpd.render_loop_max_fps = 24;
  tmpd.scaling_algorithm = ScalingAlgo::BOX_SAMPLING;
  tmpd.vom = VideoOutputMode::TEXT_ONLY;
  tmpd.volume = 1.0;
  tmpd.fullscreen = false;
  tmpd.ascii_display_chars = ASCII_STANDARD_CHAR_MAP;

  LoopType loop_type = LoopType::NO_LOOP;
  std::vector<std::string> found_media_files;

  for (std::size_t i = 0; i < paths.size(); i++) {
    if (paths[i].length() == 0) {
      std::cerr << "[tmedia] Cannot open empty path" << std::endl;
      std::cerr << parser << std::endl;
      return EXIT_FAILURE;
    }

    std::filesystem::path path(paths[i]);
    std::error_code ec;

    if (!std::filesystem::exists(path, ec)) {
      std::cerr << "[tmedia] Cannot open nonexistent path: " << paths[i] << std::endl;
      return EXIT_FAILURE;
    }

    if (std::filesystem::is_directory(path, ec)) {
      std::vector<std::string> media_file_paths;
      for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path)) {
        if (is_valid_media_file_path(entry.path().string())) {
          media_file_paths.push_back(entry.path().string());
        }
      }

      SI::natural::sort(media_file_paths);

      for (const std::string& media_file_path : media_file_paths) {
        found_media_files.push_back(media_file_path);
      }
    } else if (is_valid_media_file_path(paths[i])) {
      found_media_files.push_back(paths[i]);
    } else {
      std::cerr << "[tmedia] Cannot open path to non-media file: " << paths[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (found_media_files.size() == 0) {
    std::cerr << "[tmedia]: at least 1 media file expected in given paths. 0 found." << std::endl;
    return EXIT_FAILURE;
  }

  if (parser.get<bool>("--dump")) return program_dump_metadata(found_media_files);

  tmpd.fullscreen = parser.get<bool>("-f");
  tmpd.muted = parser.get<bool>("-m");
  if (std::optional<double> user_volume = parser.present<double>("--volume")) {
    if (*user_volume < 0.0 || *user_volume > 1.0) {
      std::cerr << "[tmedia] Received invalid volume " << *user_volume << ". Volume must be between 0.0 and 1.0 inclusive" << std::endl;
      return EXIT_FAILURE;
    }
    tmpd.volume = *user_volume;
  }

  if (std::optional<std::string> user_loop = parser.present<std::string>("--loop")) {
    if (VALID_LOOP_TYPE_ARGS.count(*user_loop) == 1) {
      loop_type = VALID_LOOP_TYPE_ARGS.at(*user_loop);
    } else {
      std::cerr << "[tmedia] Received invalid loop type '" << *user_loop << "', must be " << format_arg_map(VALID_LOOP_TYPE_ARGS, "or") << "." << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (std::optional<std::string> user_scaling_algo = parser.present<std::string>("--scaling-algo")) {
    if (VALID_SCALING_ALGO_ARGS.count(*user_scaling_algo) == 1) {
      tmpd.scaling_algorithm = VALID_SCALING_ALGO_ARGS.at(*user_scaling_algo);
    } else {
      std::cerr << "[tmedia] Unrecognized scaling algorithm '" << *user_scaling_algo << "', must be " << format_arg_map(VALID_SCALING_ALGO_ARGS, "or") << "." << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (std::optional<int> user_max_fps = parser.present<int>("--max-fps")) {
    tmpd.render_loop_max_fps = user_max_fps.value() <= 0 ? std::nullopt : user_max_fps;
  }

  if (std::optional<std::string> user_ascii_display_chars = parser.present<std::string>("--chars")) {
    tmpd.ascii_display_chars = *user_ascii_display_chars;
  }

  if (parser.get<bool>("--color"))
    tmpd.vom = parser.get<bool>("--background") ? VideoOutputMode::COLORED_BACKGROUND_ONLY : VideoOutputMode::COLORED;
  else if (parser.get<bool>("--grayscale"))
    tmpd.vom = parser.get<bool>("--background") ? VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY : VideoOutputMode::GRAYSCALE;

  tmpd.playlist = Playlist(found_media_files, loop_type);

  if (parser.get<bool>("--shuffle")) {
    tmpd.playlist.shuffle();
  }

  return tmedia(tmpd);
}

int program_print_ffmpeg_version() {
  std::cout << "libavformat: " << LIBAVFORMAT_VERSION_MAJOR << ":" << LIBAVFORMAT_VERSION_MINOR << ":" << LIBAVFORMAT_VERSION_MICRO << std::endl;
  std::cout << "libavcodec: " << LIBAVCODEC_VERSION_MAJOR << ":" << LIBAVCODEC_VERSION_MINOR << ":" << LIBAVCODEC_VERSION_MICRO << std::endl;
  std::cout << "libavutil: " << LIBAVUTIL_VERSION_MAJOR << ":" << LIBAVUTIL_VERSION_MINOR << ":" << LIBAVUTIL_VERSION_MICRO << std::endl;
  std::cout << "libswresample: " << LIBSWRESAMPLE_VERSION_MAJOR << ":" << LIBSWRESAMPLE_VERSION_MINOR << ":" << LIBSWRESAMPLE_VERSION_MICRO << std::endl;
  std::cout << "libswscale: " << LIBSWSCALE_VERSION_MAJOR << ":" << LIBSWSCALE_VERSION_MINOR << ":" << LIBSWSCALE_VERSION_MICRO << std::endl;
  return EXIT_SUCCESS;
}

int program_print_curses_version() {
  std::cout << curses_version() << std::endl;
  return EXIT_SUCCESS;
}

int program_dump_metadata(const std::vector<std::string>& files) {
  try {
    for (const std::string& file : files) {
      dump_file_info(file);
      AVFormatContext* format_context = open_format_context(file);

      MediaType media_type = media_type_from_avformat_context(format_context);
      std::string media_type_str = media_type_to_string(media_type);
      std::cerr << std::endl << "[tmedia]: Detected media Type: " << media_type_str << std::endl;

      avformat_close_input(&format_context);
    }
  } catch (const std::runtime_error& err) {
    std::cerr << "Error while dumping metadata: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
