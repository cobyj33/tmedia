#include "sigexit.h"
#include "termenv.h"

#include "wtime.h"
#include "mediaplayer.h"
#include "formatting.h"
#include "gui.h"
#include "version.h"
#include "avguard.h"
#include "avcurses.h"
#include "looptype.h"
#include "avcurses.h"
#include "ascii.h"
#include "wminiaudio.h"

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


// void render_movie_screen(PixelData& pixel_data, VideoOutputMode media_gui);
// void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode);

std::atomic<bool> INTERRUPT_RECEIVED = false;
std::atomic<unsigned int> TERM_LINES = 24;
std::atomic<unsigned int> TERM_COLS = 80;

// const int KEY_ESCAPE = 27;
// const double VOLUME_CHANGE_AMOUNT = 0.05;

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

  std::vector<std::string> _files;
  std::size_t current_file = 0;
  double _volume;
  bool _muted;
  VideoOutputMode _output_mode;
  LoopType _looped;
  double _start_time;

  
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
              "Space - Play and Pause \n"
              "Up Arrow - Increase Volume 5% \n"
              "Up Arrow - Decrease Volume 5% \n"
              "Left Arrow - Skip backward 5 seconds\n"
              "Right Arrow - Skip forward 5 seconds \n"
              "Escape or Backspace - End Playback \n"
              "'0' - Restart playback\n"
              "'1' through '9' - Skip to the timestamp at n/10 of the duration (similar to youtube)\n"
              "c - Change to color mode on supported terminals \n"
              "g - Change to grayscale mode \n"
              "b - Change to Background Mode on supported terminals if in Color or Grayscale mode \n");

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

  double volume = 1.0;


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
      std::cerr << "[ascii_video] Cannot open invalid path: " << paths[i] << std::endl;
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

  if (colors && grayscale) {
    grayscale = false;
  }

  if (background && !(colors || grayscale)) {
    colors = true;
  }

  ncurses_init();

  VideoOutputMode mode = get_video_output_mode_from_params(colors, grayscale, background);
  MediaGUI media_gui;
  media_gui.set_video_output_mode(mode);

  try {
    for (std::size_t i = 0; i < files.size(); i++) {
      MediaPlayer player(files[i], media_gui);
      player.muted = mute;
      player.volume = volume;
      player.start(0.0);
      
      if (player.has_error()) {
        std::cout << "[ascii_video]: Media Player Error: " << player.get_error() << std::endl;
        return EXIT_FAILURE;
      }

      if (INTERRUPT_RECEIVED) {
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


// /**
//  * @brief The callback called by miniaudio once the connected audio device requests audio data
//  * 
//  * @param pDevice The miniaudio audio device representation
//  * @param pOutput The memory buffer to write frameCount samples into
//  * @param pInput Irrelevant for us :)
//  * @param sampleCount The number of samples requested by miniaudio
//  */
// void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
// {
//   MediaPlayer* player = (MediaPlayer*)(pDevice->pUserData);
//   std::lock_guard<std::mutex> mutex_lock_guard(player->buffer_read_mutex);

//   if (!player->clock.is_playing() || !player->audio_buffer->can_read(frameCount)) {
//     float* pFloatOutput = (float*)pOutput;
//     for (ma_uint32 i = 0; i < frameCount; i++) {
//       pFloatOutput[i] = 0.0001 * sin(0.10 * i);
//     }
//   } else if (player->audio_buffer->can_read(frameCount)) {
//     player->audio_buffer->read_into(frameCount, (float*)pOutput);
//   }

//   (void)pInput;
// }

// void render_movie_screen(PixelData& pixel_data, VideoOutputMode output_mode) {
//   print_pixel_data(pixel_data, 0, 0, COLS, LINES, output_mode);
// }


// void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode) {
//   if (output_mode != VideoOutputMode::TEXT_ONLY && !has_colors()) {
//     throw std::runtime_error("Attempted to print colored text in terminal that does not support color");
//   }

//   PixelData bounded = pixel_data.bound(bounds_width, bounds_height);
//   int image_start_row = bounds_row + std::abs(bounded.get_height() - bounds_height) / 2;
//   int image_start_col = bounds_col + std::abs(bounded.get_width() - bounds_width) / 2; 

//   bool background_only = output_mode == VideoOutputMode::COLORED_BACKGROUND_ONLY || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;

//   for (int row = 0; row < bounded.get_height(); row++) {
//     for (int col = 0; col < bounded.get_width(); col++) {
//       const RGBColor& target_color = bounded.at(row, col);
//       const char target_char = background_only ? ' ' : get_char_from_rgb(AsciiImage::ASCII_STANDARD_CHAR_MAP, target_color);
      
//       if (output_mode == VideoOutputMode::TEXT_ONLY) {
//         mvaddch(image_start_row + row, image_start_col + col, target_char);
//       } else {
//         const int color_pair = get_closest_ncurses_color_pair(target_color);
//         mvaddch(image_start_row + row, image_start_col + col, target_char | COLOR_PAIR(color_pair));
//       }

//     }
//   }

// }