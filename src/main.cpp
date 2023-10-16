#include "wtime.h"
#include "mediaplayer.h"
#include "formatting.h"
#include "gui.h"
#include "version.h"
#include "avguard.h"
#include "avcurses.h"
#include "sigexit.h"


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <atomic>
#include <csignal>

#include <argparse.hpp>
#include <natural_sort.hpp>

extern "C" {
#include "curses.h"
#include <libavutil/log.h>
#include <libavformat/version.h>
#include <libavutil/version.h>
#include <libavcodec/version.h>
#include <libswresample/version.h>
#include <libswscale/version.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
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

  // common signal handlers
  // i literally just took this from the git git repo (sigchain.c) 
  std::signal(SIGINT, ascii_video_signal_handler); // interrupted (usually with Ctrl+C)
	std::signal(SIGTERM, ascii_video_signal_handler); // politely asking for termination
  
	// std::signal(SIGQUIT, ascii_video_signal_handler); I don't know if I should handle this,
  // as I'd want quitting if ascii_video does happen to actually break

	// std::signal(SIGHUP, ascii_video_signal_handler); This is when the user's terminal is disconnected or quit, I do want
  // to handle this, but I'm sure that it has some exceptions with ncurses use

  argparse::ArgumentParser parser("ascii_video", ASCII_VIDEO_VERSION);

  parser.add_argument("paths")   
    .help("The the paths to files or directories to be played. "
    "Multiple files will be played one after the other. "
    "Directories will be expanded so any media files inside them will be played.")
    .nargs(argparse::nargs_pattern::any);

  const std::string controls = std::string("-------CONTROLS-----------\n"
              "SPACE - Toggle Playback \n"
              "Up Arrow - Increase Volume 5% \n"
              "Up Arrow - Decrease Volume 5% \n"
              "Left Arrow - Skip backward 5 seconds\n"
              "Right Arrow - Skip forward 5 seconds \n"
              "Escape or Backspace - End Playback \n"
              "C - Change to color mode on supported terminals \n"
              "G - Change to grayscale mode \n"
              "B - Change to Background Mode on supported terminals if in Color or Grayscale mode \n");

  parser.add_description(controls);

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

  parser.add_argument("-q", "--quiet")
    .help("Silence any warnings outputted before the beginning of playback")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("-t", "--time")
    .help("Choose the time to start media playback");

  parser.add_argument("-m", "--mute")
    .help("Mute the audio playback")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("--volume")
    .help("Set initial volume (must be between 0.0 and 1.0)")
    .scan<'g', double>();

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
  catch (std::runtime_error const& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << parser;
    return EXIT_FAILURE;
  }

  double start_time = 0.0;
  double volume = 1.0;
  std::string inputted_start_time = "00:00";


  std::vector<std::string> paths = parser.get<std::vector<std::string>>("paths");
  bool colors = parser.get<bool>("-c");
  bool grayscale = !colors && parser.get<bool>("-g");
  bool background = parser.get<bool>("-b");
  bool dump = parser.get<bool>("-d");
  bool quiet = parser.get<bool>("-q");
  bool mute = parser.get<bool>("-m");
  bool print_ffmpeg_version = parser.get<bool>("--ffmpeg-version");
  bool print_curses_version = parser.get<bool>("--curses-version");

  if (std::optional<double> user_volume = parser.present<double>("--volume")) {
    volume = *user_volume;
  }

  if (std::optional<std::string> user_start_time = parser.present<std::string>("--time")) {
    inputted_start_time = *user_start_time;
  }

  if (print_ffmpeg_version) {
    std::cout << "libavformat: " << LIBAVFORMAT_VERSION_MAJOR << ":" << LIBAVFORMAT_VERSION_MINOR << ":" << LIBAVFORMAT_VERSION_MICRO << std::endl;
    std::cout << "libavcodec: " << LIBAVCODEC_VERSION_MAJOR << ":" << LIBAVCODEC_VERSION_MINOR << ":" << LIBAVCODEC_VERSION_MICRO << std::endl;
    std::cout << "libavutil: " << LIBAVUTIL_VERSION_MAJOR << ":" << LIBAVUTIL_VERSION_MINOR << ":" << LIBAVUTIL_VERSION_MICRO << std::endl;
    std::cout << "libswresample: " << LIBSWRESAMPLE_VERSION_MAJOR << ":" << LIBSWRESAMPLE_VERSION_MINOR << ":" << LIBSWRESAMPLE_VERSION_MICRO << std::endl;
    std::cout << "libswscale: " << LIBSWSCALE_VERSION_MAJOR << ":" << LIBSWSCALE_VERSION_MINOR << ":" << LIBSWSCALE_VERSION_MICRO << std::endl;
    return EXIT_SUCCESS;
  }

  if (print_curses_version) {
    std::cout << curses_version() << std::endl;
    return EXIT_SUCCESS;
  }

  if (paths.size() == 0) {
    std::cerr << "[ascii_video]: at least 1 path expected. 0 found. Path can be to directory or media file" << std::endl;
    std::cerr << parser << std::endl;
    return EXIT_FAILURE;
  }

  std::vector<std::string> files;
  for (std::size_t i = 0; i < paths.size(); i++) {
    if (paths[i].length() == 0) {
      std::cerr << "[ascii_video] Cannot open empty path" << std::endl;
      return EXIT_FAILURE;
    }

    std::filesystem::path path(paths[i]);
    std::error_code ec;

    if (!std::filesystem::exists(path, ec)) {
      std::cerr << "[ascii_video] Cannot open invalid path: " << paths[i] << " " << ec << std::endl;
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
        files.push_back(media_file_path);
      }

    } else if (is_valid_media_file_path(paths[i])) {
      files.push_back(paths[i]);
    } else {
      std::cerr << "[ascii_video] Cannot open path to non-media file: " << paths[i] << " " << ec << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (files.size() == 0) {
    std::cerr << "[ascii_video]: at least 1 media file expected. 0 found." << std::endl;
    std::cerr << parser << std::endl;
    return EXIT_FAILURE;
  }

  if (volume < 0.0 || volume > 1.0) {
    std::cerr << "[ascii_video]: volume must be in between 0.0 and 1.0 inclusive (got " << volume << ")" << std::endl;
    std::cerr << parser << std::endl;
    return EXIT_FAILURE;
  }

  if (dump) {
    for (std::size_t i = 0; i < files.size(); i++) {
      dump_file_info(files[i]);
      AVFormatContext* format_context = open_format_context(files[i]);

      MediaType media_type = media_type_from_avformat_context(format_context);
      std::string media_type_str = media_type_to_string(media_type);
      std::cerr << std::endl << "[ascii_video]: Detected media Type: " << media_type_str << std::endl;

      avformat_free_context(format_context);
    }

    return EXIT_SUCCESS;
  }

  if (is_duration(inputted_start_time)) {
    start_time = parse_duration(inputted_start_time);
  } else if (is_int_str(inputted_start_time)) {
    start_time = std::stoi(inputted_start_time);
  } else {
    std::cerr << "[ascii_video]: Inputted time must be in seconds, H:MM:SS, or M:SS format (got \"" << inputted_start_time << "\" )" << std::endl;
    return EXIT_FAILURE;
  }

  if (start_time < 0.0) { // Catch errors in out of bounds times
    std::cerr << "[ascii_video]: Cannot start media player at a negative time ( got time " << double_to_fixed_string(start_time, 2) << "  from input " << inputted_start_time << " )" << std::endl;
    return EXIT_FAILURE;
  }

  for (std::size_t i = 0; i < files.size(); i++) {
    double file_duration = get_file_duration(files[i]);
    if (start_time >= file_duration && file_duration >= 0.0) { 
      std::cerr << "[ascii_video]: Cannot start media file at a time greater than media's duration ( got time " << double_to_fixed_string(start_time, 2) << " seconds from input " << inputted_start_time << ". File ends at " << double_to_fixed_string(file_duration, 2) << " seconds (" << format_duration(file_duration) << ") ) "  << std::endl;
      return EXIT_FAILURE;
    }
  }

  ncurses_init();
  std::string warnings;

  if (colors && grayscale) {
    warnings += "[ascii_video]: WARNING: Cannot have both colored (-c, --color) and grayscale (-g, --gray) output. Falling back to colored output\n";
    grayscale = false;
  }

  if ( (colors || grayscale) && !has_colors()) {
    warnings += "[ascii_video]: WARNING: Terminal does not support colored output\n";
    colors = false;
    grayscale = false;
  }

  if (background && !(colors || grayscale) ) {
    if (!has_colors()) {
      warnings += "[ascii_video]: WARNING: Cannot use background option as colors are not available in this terminal. Falling back to pure text output\n";
      background = false;
    } else {
      warnings += "[ascii_video]: WARNING: Cannot only show background without either color (-c, --color) or grayscale(-g, --gray, --grayscale) options selected. Falling back to colored output\n";
      colors = true;
      grayscale = false;
    }
  }

  if ( (grayscale || colors) && has_colors() && !can_change_color()) {
    warnings += "WARNING: Terminal does not support changing colored output data, colors may not be as accurate as expected\n";
  }

  if (warnings.length() > 0 && !quiet) {
    printw("%s\n\n%s\n", warnings.c_str(), "Press any key to continue");
    refresh();
    getch();
  }
  erase();

  VideoOutputMode mode = get_video_output_mode_from_params(colors, grayscale, background);
  MediaGUI media_gui;
  media_gui.set_video_output_mode(mode);

  try {
    for (std::size_t i = 0; i < files.size(); i++) {
      MediaPlayer player(files[i], media_gui);
      player.muted = mute;
      player.volume = volume;
      player.start(start_time);
      

      if (player.has_error()) {
        std::cout << "[ascii_video]: Media Player Error: " << player.get_error() << std::endl;
        return EXIT_FAILURE;
      }

      if (should_sig_exit()) {
        break;
      }
    }
  } catch (std::exception const &e) {
    std::cout << "[ascii_video]: Error while starting media player: " << e.what() << std::endl;
    ncurses_uninit();
    return EXIT_FAILURE;
  }

  ncurses_uninit();
  return EXIT_SUCCESS;
}


