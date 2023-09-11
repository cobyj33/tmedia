#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <filesystem>

#include "color.h"
#include "icons.h"
#include "pixeldata.h"
#include "scale.h"
#include "media.h"
#include "formatting.h"
#include "wtime.h"
#include "gui.h"
#include "version.h"
#include "avguard.h"

#include <termcolor.h>


#include <argparse.hpp>


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

bool ncurses_initialized = false;

bool is_valid_path(const char* path);
void ncurses_init();
void ncurses_uninit();

void on_terminate() {
  if (ncurses_initialized) {
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

  argparse::ArgumentParser parser("ascii_video", ASCII_VIDEO_VERSION);

  parser.add_argument("file")   
    .help("The file to be played")
    .nargs(0, 1);

  const std::string controls = std::string("-------CONTROLS-----------\n") +
              "SPACE - Toggle Playback \n" +
              "Left Arrow - Skip backward 5 seconds\n" +
              "Right Arrow - Skip forward 5 seconds \n" +
              "Escape or Backspace - End Playback \n"
              "C - Change to color mode on supported terminals \n"
              "G - Change to grayscale mode \n"
              "B - Change to Background Mode on supported terminals if in Color or Grayscale mode \n";

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
    .help("Print metadata about the file")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("-q", "--quiet")
    .help("Silence any warnings outputted before the beginning of playback")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("-t", "--time")
    .help("Choose the time to start media playback")
    .default_value(std::string("00:00"));


  parser.add_argument("-m", "--mute")
    .help("Mute the audio playback")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("--no-audio")
    .help("Remove all audio playback from the player")
    .default_value(false)
    .implicit_value(true);

  parser.add_argument("--ffmpeg-version")
    .help("Print the version of linked FFmpeg libraries")
    .default_value(false)
    .implicit_value(true);


  parser.add_argument("--curses-version")
    .help("Print the version of linked Curses libraries")
    .default_value(false)
    .implicit_value(true);

  // parser.add_argument("-i", "--image")
  //   .help("Consume the media as an image")
  //   .default_value(false)
  //   .implicit_value(true);

  try {
    parser.parse_args(argc, argv);
  }
  catch (std::runtime_error const& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << parser;
    return EXIT_FAILURE;
  }

  std::vector<std::string> file_inputs = parser.get<std::vector<std::string>>("file");
  
  double start_time = 0.0;
  bool colors = parser.get<bool>("-c");
  bool grayscale = !colors && parser.get<bool>("-g");
  bool background = parser.get<bool>("-b");
  bool dump = parser.get<bool>("-d");
  bool quiet = parser.get<bool>("-q");
  bool mute = parser.get<bool>("-m");
  bool print_ffmpeg_version = parser.get<bool>("--ffmpeg-version");
  bool print_curses_version = parser.get<bool>("--curses-version");
  bool audio_enabled = !parser.get<bool>("--no-audio");
  std::string inputted_start_time = parser.get<std::string>("-t");

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


  if (file_inputs.size() == 0) {
    std::cerr << "Error: 1 media file input expected. 0 provided." << std::endl;
    std::cerr << parser << std::endl;
    return EXIT_FAILURE;
  }
  std::string file = file_inputs[0];


  if (dump) {
    dump_file_info(file.c_str());
    return EXIT_SUCCESS;
  }

  if (!is_valid_path(file.c_str()) && file.length() > 0) {
    std::cerr << "[ascii_video] Cannot open invalid path: " << file << std::endl;
    return EXIT_FAILURE;
  }

  const std::filesystem::path path(file);
  std::error_code ec; // For using the non-throwing overloads of functions below.
  if (std::filesystem::is_directory(path, ec))
  { 
    std::cerr << "[ascii_video] Given path " << file << " is a directory." << std::endl;
    return EXIT_FAILURE; 
  }

  if (inputted_start_time.length() > 0) {
    if (is_duration(inputted_start_time)) {
      start_time = parse_duration(inputted_start_time);
    } else if (is_int_str(inputted_start_time)) {
      start_time = std::stoi(inputted_start_time);
    } else {
      std::cerr << "[ascii_video] Inputted time must be in seconds, H:MM:SS, or M:SS format (got \"" << inputted_start_time << "\" )" << std::endl;
      return EXIT_FAILURE;
    }
  }

  double file_duration = get_file_duration(file.c_str());
  if (start_time < 0) { // Catch errors in out of bounds times
    std::cerr << "Cannot start file at a negative time ( got time " << double_to_fixed_string(start_time, 2) << "  from input " + inputted_start_time + " )" << std::endl;
    return EXIT_FAILURE;
  } else if (start_time >= file_duration && file_duration >= 0.0) { 
    std::cerr << "Cannot start file at a time greater than duration of media file ( got time " << double_to_fixed_string(start_time, 2) << " seconds from input " << inputted_start_time << ". File ends at " << double_to_fixed_string(file_duration, 2) << " seconds (" << format_duration(file_duration) << ") ) "  << std::endl;
    return EXIT_FAILURE;
  }

  ncurses_init();
  std::string warnings;

  if (colors && grayscale) {
    warnings += "WARNING: Cannot have both colored (-c, --color) and grayscale (-g, --gray) output. Falling back to colored output\n";
    grayscale = false;
  }

  if ( (colors || grayscale) && !has_colors()) {
    warnings += "WARNING: Terminal does not support colored output\n";
    colors = false;
    grayscale = false;
  }

  if (background && !(colors || grayscale) ) {
    if (!has_colors()) {
      warnings += "WARNING: Cannot use background option as colors are not available in this terminal. Falling back to pure text output\n";
      background = false;
    } else {
      warnings += "WARNING: Cannot only show background without either color (-c, --color) or grayscale(-g, --gray, --grayscale) options selected. Falling back to colored output\n";
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

  if (mode == VideoOutputMode::GRAYSCALE || mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY) {
    ncurses_initialize_grayscale_color_palette();
  }

  MediaGUI media_gui;
  media_gui.set_video_output_mode(mode);
  MediaPlayerConfig config(audio_enabled);
  MediaPlayer player(file.c_str(), media_gui, config);
  player.muted = mute;
  player.start(start_time);
  ncurses_uninit();

  if (player.media_type == MediaType::VIDEO || player.media_type == MediaType::AUDIO) {
    std::cout << media_type_to_string(player.media_type) << " Player ended at " << format_duration(player.get_time(system_clock_sec())) << " / " << format_duration(player.get_duration()) << std::endl;
  }
  return EXIT_SUCCESS;
}

bool is_valid_path(const char* path) {
  FILE* file;
  file = fopen(path, "r");

  if (file != nullptr) {
    fclose(file);
    return true;
  }

  return false;
}

void ncurses_init() {
  if (ncurses_initialized) {
    throw std::runtime_error("ncurses attempted to be initialized although it has already been initialized");
  }
  ncurses_initialized = true;
  initscr();
  start_color();
  ncurses_initialize_color_palette();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, true);
}

void ncurses_uninit() {
  if (!ncurses_initialized) {
    throw std::runtime_error("ncurses attempted to be uninitialized although it has never been initialized");
  }
  ncurses_initialized = false;
  use_default_colors();
  keypad(stdscr, false);
  nocbreak();
  echo();
  endwin();
}