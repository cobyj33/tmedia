#include "signalstate.h"

#include "wtime.h"
#include "mediafetcher.h"
#include "formatting.h"
#include "version.h"
#include "avguard.h"
#include "avcurses.h"
#include "looptype.h"
#include "avcurses.h"
#include "ascii.h"
#include "wminiaudio.h"
#include "sleep.h"
#include "wmath.h"
#include "boiler.h"
#include "asv_string.h"
#include "ascii_video.h"

#include <cstdlib>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <csignal>

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
    std::cerr << "Terminated due to exception: " << ex.what() << std::endl;
  }

  std::abort();
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
  
  argparse::ArgumentParser parser("ascii_video", ASCII_VIDEO_VERSION);

  parser.add_argument("paths")
    .help("The the paths to files or directories to be played.\n"
    "Multiple files will be played one after the other.\n"
    "Directories will be expanded so any media files inside them will be played.")
    .nargs(argparse::nargs_pattern::any);

  parser.add_description(ASCII_VIDEO_CONTROLS_USAGE);

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

  parser.add_argument("-m", "--mute")
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
    .help("Set the loop type of the player ('none', 'repeat', 'repeat-one')");

  parser.add_argument("--max-fps")
    .help("Set the maximum rendering fps of ascii_video")
    .scan<'i', int>();

  parser.add_argument("--scaling-algo")
    .help("Set the scaling algorithm to use when rendering frames  ('box-sampling', 'nearest-neighbor')");

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
    std::cerr << err.what() << std::endl;
    std::cerr << parser;
    return EXIT_FAILURE;
  }

  if (parser.get<bool>("--ffmpeg-version")) return program_print_ffmpeg_version();
  if (parser.get<bool>("--curses-version")) return program_print_curses_version();
  std::vector<std::string> paths = parser.get<std::vector<std::string>>("paths");

  if (paths.size() == 0) {
    std::cerr << "[ascii_video]: at least 1 path expected. 0 found. Path can be to directory or media file" << std::endl;
    std::cerr << parser << std::endl;
    return EXIT_FAILURE;
  }

  AsciiVideoProgramData avpd;
  avpd.loop_type = LoopType::NO_LOOP;
  avpd.muted = false;
  avpd.render_loop_max_fps = 24;
  avpd.scaling_algorithm = ScalingAlgo::BOX_SAMPLING;
  avpd.loop_type = LoopType::NO_LOOP;
  avpd.vom = VideoOutputMode::TEXT_ONLY;
  avpd.volume = 1.0;
  avpd.fullscreen = false;

  for (std::size_t i = 0; i < paths.size(); i++) {
    if (paths[i].length() == 0) {
      std::cerr << "[ascii_video] Cannot open empty path" << std::endl;
      std::cerr << parser << std::endl;
      return EXIT_FAILURE;
    }

    std::filesystem::path path(paths[i]);
    std::error_code ec;

    if (!std::filesystem::exists(path, ec)) {
      std::cerr << "[ascii_video] Cannot open nonexistent path: " << paths[i] << std::endl;
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
        avpd.files.push_back(media_file_path);
      }
    } else if (is_valid_media_file_path(paths[i])) {
      avpd.files.push_back(paths[i]);
    } else {
      std::cerr << "[ascii_video] Cannot open path to non-media file: " << paths[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (avpd.files.size() == 0) {
    std::cerr << "[ascii_video]: at least 1 media file expected in given paths. 0 found." << std::endl;
    return EXIT_FAILURE;
  }

  if (parser.get<bool>("--dump")) return program_dump_metadata(avpd.files);

  avpd.fullscreen = parser.get<bool>("-f");
  avpd.muted = parser.get<bool>("-m");
  if (std::optional<double> user_volume = parser.present<double>("--volume")) {
    if (*user_volume < 0.0 || *user_volume > 1.0) {
      std::cerr << "[ascii_video] Received invalid volume " << *user_volume << ". Volume must be between 0.0 and 1.0 inclusive" << std::endl;
      return EXIT_FAILURE;
    }
    avpd.volume = *user_volume;
  }

  if (std::optional<std::string> user_loop = parser.present<std::string>("--loop")) {
    if (*user_loop == "none") {
      avpd.loop_type = LoopType::NO_LOOP;
    } else if (*user_loop == "repeat") {
      avpd.loop_type = LoopType::REPEAT;
    } else if (*user_loop == "repeat-one") {
      avpd.loop_type = LoopType::REPEAT_ONE;
    } else {
      std::cerr << "[ascii_video] Received invalid loop type '" << *user_loop << "', must be 'none', 'repeat', or 'repeat-one'" << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (std::optional<std::string> user_scaling_algo = parser.present<std::string>("--scaling-algo")) {
    if (*user_scaling_algo == "nearest-neighbor") {
      avpd.scaling_algorithm = ScalingAlgo::NEAREST_NEIGHBOR;
    }
    else if (*user_scaling_algo == "box-sampling") {
      avpd.scaling_algorithm = ScalingAlgo::BOX_SAMPLING;
    } else {
      std::cerr << "[ascii_video] Unrecognized scaling algorithm '" + *user_scaling_algo + "', must be 'nearest-neighbor' or 'box-sampling'" << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (std::optional<int> user_max_fps = parser.present<int>("--max-fps")) {
    avpd.render_loop_max_fps = user_max_fps.value() <= 0 ? std::nullopt : user_max_fps;
  }

  if (parser.get<bool>("--color"))
    avpd.vom = parser.get<bool>("--background") ? VideoOutputMode::COLORED_BACKGROUND_ONLY : VideoOutputMode::COLORED;
  else if (parser.get<bool>("--grayscale"))
    avpd.vom = parser.get<bool>("--background") ? VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY : VideoOutputMode::GRAYSCALE;

  return ascii_video(avpd);
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
      std::cerr << std::endl << "[ascii_video]: Detected media Type: " << media_type_str << std::endl;

      avformat_close_input(&format_context);
    }
  } catch (const std::runtime_error& err) {
    std::cerr << "Error while dumping metadata: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
