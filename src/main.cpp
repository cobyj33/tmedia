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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <atomic>
#include <csignal>
#include <thread>

#include <argparse.hpp>
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
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

enum class VideoOutputMode {
  COLORED,
  GRAYSCALE,
  COLORED_BACKGROUND_ONLY,
  GRAYSCALE_BACKGROUND_ONLY,
  TEXT_ONLY,
};

enum class JustifyStrings {
  SPACE_BETWEEN
};

// void wprintstr_list(WINDOW* window, int y, int x, int width, 
void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode);
void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void wprint_progress_bar(WINDOW* window, int y, int x, int width, int height, double percentage);
void wprint_playback_bar(WINDOW* window, int y, int x, int width, double time, double duration);

bool INTERRUPT_RECEIVED = false;
const int KEY_ESCAPE = 27;
const double VOLUME_CHANGE_AMOUNT = 0.05;

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

  std::vector<std::string> files;
  std::size_t current_file = 0;
  double volume = 1.0;
  bool muted = false;
  VideoOutputMode vom  = VideoOutputMode::TEXT_ONLY;
  LoopType loop_type = LoopType::NO_LOOP;
  
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
              "Escape or Backspace - End Program \n"
              "'0' - Restart playback\n"
              "'1' through '9' - Skip to the timestamp at n/10 of the duration (similar to youtube)\n"
              "c - Change to color mode on supported terminals \n"
              "g - Change to grayscale mode \n"
              "b - Change to Background Mode on supported terminals if in Color or Grayscale mode \n"
              "n - Go to next media file\n"
              "p - Go to previous media file\n"
              "l - Switch looping type of playback\n");

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

  std::vector<std::string> paths = parser.get<std::vector<std::string>>("paths");
  bool colors = parser.get<bool>("-c");
  bool grayscale = !colors && parser.get<bool>("-g");
  bool background = parser.get<bool>("-b");
  bool dump = parser.get<bool>("-d");
  bool print_ffmpeg_version = parser.get<bool>("--ffmpeg-version");
  bool print_curses_version = parser.get<bool>("--curses-version");
  bool fullscreen = parser.get<bool>("-f");

  muted = parser.get<bool>("-m");
  if (std::optional<double> user_volume = parser.present<double>("--volume")) {
    volume = *user_volume;
  }

  if (std::optional<std::string> user_loop = parser.present<std::string>("--loop")) {
    if (*user_loop == "none") {
      loop_type = LoopType::NO_LOOP;
    } else if (*user_loop == "repeat") {
      loop_type = LoopType::REPEAT;
    } else if (*user_loop == "repeat-one") {
      loop_type = LoopType::REPEAT_ONE;
    } else {
      std::cerr << "[ascii_video] Received invalid loop type '" << *user_loop << "', must be either 'none', 'repeat', or 'repeat-one'" << std::endl;
      std::cerr << parser << std::endl;
      return EXIT_FAILURE;
    }
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

  if (volume < 0.0 || volume > 1.0) {
    std::cerr << "[ascii_video]: volume must be in between 0.0 and 1.0 inclusive (got " << volume << ")" << std::endl;
    std::cerr << parser << std::endl;
    return EXIT_FAILURE;
  }

  if (paths.size() == 0) {
    std::cerr << "[ascii_video]: at least 1 path expected. 0 found. Path can be to directory or media file" << std::endl;
    std::cerr << parser << std::endl;
    return EXIT_FAILURE;
  }

  for (std::size_t i = 0; i < paths.size(); i++) {
    if (paths[i].length() == 0) {
      std::cerr << "[ascii_video] Cannot open empty path" << std::endl;
      std::cerr << parser << std::endl;
      return EXIT_FAILURE;
    }

    std::filesystem::path path(paths[i]);
    std::error_code ec;

    if (!std::filesystem::exists(path, ec)) {
      std::cerr << "[ascii_video] Cannot open invalid path: " << paths[i] << std::endl;
      std::cerr << parser << std::endl;
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
      std::cerr << parser << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (files.size() == 0) {
    std::cerr << "[ascii_video]: at least 1 media file expected. 0 found." << std::endl;
    std::cerr << parser << std::endl;
    return EXIT_FAILURE;
  }

  if (dump) {
    for (const std::string& file : files) {
      dump_file_info(file);
      AVFormatContext* format_context = open_format_context(file);

      MediaType media_type = media_type_from_avformat_context(format_context);
      std::string media_type_str = media_type_to_string(media_type);
      std::cerr << std::endl << "[ascii_video]: Detected media Type: " << media_type_str << std::endl;

      avformat_close_input(&format_context);
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

  if (colors) {
    ncurses_set_color_palette(AVNCursesColorPalette::RGB);
    vom  = background ? VideoOutputMode::COLORED_BACKGROUND_ONLY : VideoOutputMode::COLORED;
  } else if (grayscale) {
    ncurses_set_color_palette(AVNCursesColorPalette::GRAYSCALE);
    vom  = grayscale ? VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY : VideoOutputMode::GRAYSCALE;
  }

  const int RENDER_LOOP_SLEEP_TIME_MS = 41;
  bool full_exit = false;

  while (!INTERRUPT_RECEIVED && !full_exit && current_file < files.size()) {
    MediaFetcher fetcher(files[current_file]);
    std::unique_ptr<ma_device_w> audio_device;
    int next_file = playback_get_next(current_file, files.size(), loop_type);

    if (fetcher.has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      ma_device_config config = ma_device_config_init(ma_device_type_playback);
      config.playback.format  = ma_format_f32;
      config.playback.channels = fetcher.media_decoder->get_nb_channels();
      config.sampleRate = fetcher.media_decoder->get_sample_rate();       
      config.dataCallback = audioDataCallback;   
      config.pUserData = &fetcher;

      audio_device = std::make_unique<ma_device_w>(&config);
      audio_device->start();
      audio_device->set_volume(volume);
    }

    fetcher.begin();

    try {
      PixelData frame;
      while (fetcher.in_use) { // never break without setting in_use to false

        if (INTERRUPT_RECEIVED) {
          fetcher.in_use = false;
          break;
        }

        double requested_jump_time = 0.0;
        bool requested_jump = false;
        double timestamp = 0.0;

        {
          std::lock_guard<std::mutex> _alter_lock(fetcher.alter_mutex);
          // std::lock_guard<std::mutex> _buffer_lock(fetcher.audio_buffer_mutex); jump time calls are now handled outside of input loop
          const double current_system_time = system_clock_sec();
          requested_jump_time = fetcher.get_time(current_system_time);
          timestamp = fetcher.get_time(current_system_time);
          frame = fetcher.frame;

          if (fetcher.media_type != MediaType::IMAGE && fetcher.get_time(current_system_time) >= fetcher.get_duration()) {
            fetcher.in_use = false;
            break;
          }

          int input = getch();

          while (input != ERR) { // Go through and process all the batched input
            if (input == KEY_ESCAPE || input == KEY_BACKSPACE || input == 127 || input == '\b') {
              fetcher.in_use = false;
              full_exit = true;
              break; // break out of input != ERR
            }

            if (input == KEY_RESIZE) {
              erase();
              refresh();
            }

            if ((input == 'c' || input == 'C') && has_colors() && can_change_color()) { // Change from current video mode to colored version
              switch (vom) {
                case VideoOutputMode::COLORED:
                  vom = VideoOutputMode::TEXT_ONLY;
                  break;
                case VideoOutputMode::GRAYSCALE:
                  vom = VideoOutputMode::COLORED;
                  ncurses_set_color_palette(AVNCursesColorPalette::RGB);
                  break;
                case VideoOutputMode::COLORED_BACKGROUND_ONLY:
                  vom = VideoOutputMode::TEXT_ONLY;
                  break;
                case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: 
                  vom = VideoOutputMode::COLORED_BACKGROUND_ONLY;
                  ncurses_set_color_palette(AVNCursesColorPalette::RGB);
                  break; 
                case VideoOutputMode::TEXT_ONLY:
                  vom = VideoOutputMode::COLORED;
                  ncurses_set_color_palette(AVNCursesColorPalette::RGB);
                  break;
              }
            }

            if ((input == 'g' || input == 'G') && has_colors() && can_change_color()) {
              switch (vom) {
                case VideoOutputMode::COLORED:
                  vom = VideoOutputMode::GRAYSCALE;
                  ncurses_set_color_palette(AVNCursesColorPalette::GRAYSCALE);
                  break;
                case VideoOutputMode::GRAYSCALE:
                  vom = VideoOutputMode::TEXT_ONLY;
                  break;
                case VideoOutputMode::COLORED_BACKGROUND_ONLY:
                  vom = VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;
                  ncurses_set_color_palette(AVNCursesColorPalette::GRAYSCALE);
                  break;
                case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: 
                  vom = VideoOutputMode::TEXT_ONLY;
                  break; 
                case VideoOutputMode::TEXT_ONLY:
                  vom = VideoOutputMode::GRAYSCALE;
                  ncurses_set_color_palette(AVNCursesColorPalette::GRAYSCALE);
                  break;
              }
            }

            if ((input == 'b' || input == 'B') && has_colors() && can_change_color()) {
              switch (vom) {
                case VideoOutputMode::COLORED:
                  vom = VideoOutputMode::COLORED_BACKGROUND_ONLY;
                  break;
                case VideoOutputMode::GRAYSCALE:
                  vom = VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;
                  break;
                case VideoOutputMode::COLORED_BACKGROUND_ONLY:
                  vom = VideoOutputMode::COLORED;
                  break;
                case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: 
                  vom = VideoOutputMode::GRAYSCALE;
                  break; 
                case VideoOutputMode::TEXT_ONLY: break;
              }
            }


            if (input == 'l' || input == 'L') {
              switch (loop_type) {
                case LoopType::NO_LOOP: loop_type = LoopType::REPEAT; break;
                case LoopType::REPEAT: loop_type = LoopType::REPEAT_ONE; break;
                case LoopType::REPEAT_ONE: loop_type = LoopType::NO_LOOP; break;
              }
              next_file = playback_get_next(current_file, files.size(), loop_type);
            }

            if (input == 'n' || input == 'N') {
              if (loop_type == LoopType::REPEAT_ONE) {
                loop_type = LoopType::REPEAT;
                next_file = playback_get_next(current_file, files.size(), loop_type);
              }
              fetcher.in_use = false;
            }

            if (input == 'p' || input == 'P') {
              if (loop_type == LoopType::REPEAT_ONE)
                loop_type = LoopType::REPEAT;
              next_file = playback_get_previous(current_file, files.size(), loop_type);
              fetcher.in_use = false;
            }

            if (input == 'f' || input == 'F') {
              erase();
              fullscreen = !fullscreen;
            }

            if (input == KEY_UP && audio_device) {
              volume = clamp(volume + VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
              audio_device->set_volume(volume);
            }

            if (input == KEY_DOWN && audio_device) {
              volume = clamp(volume - VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
              audio_device->set_volume(volume);
            }

            if (input == 'm' || input == 'M') {
              muted = !muted;
            }

            if (input == ' ' && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              switch (fetcher.clock.is_playing()) {
                case true:  {
                  if (audio_device) audio_device->stop();
                  fetcher.clock.stop(current_system_time);
                } break;
                case false: {
                  if (audio_device) audio_device->start();
                  fetcher.clock.resume(current_system_time);
                } break;
              }
            }

            if (input == KEY_LEFT && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time -= 5.0;
            }

            if (input == KEY_RIGHT && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time += 5.0;
            }

            if (input == '0'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = 0.0;
            }

            if (input == '1'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = fetcher.get_duration() * 1.0 / 10.0;
            }

            if (input == '2'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = fetcher.get_duration() * 2.0 / 10.0;
            }

            if (input == '3'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = fetcher.get_duration() * 3.0 / 10.0;
            }

            if (input == '4'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = fetcher.get_duration() * 4.0 / 10.0;
            }

            if (input == '5'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = fetcher.get_duration() * 5.0 / 10.0;
            }

            if (input == '6'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = fetcher.get_duration() * 6.0 / 10.0;
            }

            if (input == '7'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = fetcher.get_duration() * 7.0 / 10.0;
            }

            if (input == '8'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = fetcher.get_duration() * 8.0 / 10.0;
            }

            if (input == '9'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              requested_jump = true;
              requested_jump_time = fetcher.get_duration() * 9.0 / 10.0;
            }

            input = getch();
          } // Ending of "while (input != ERR)"
        } // end of locked context

        if (requested_jump) {
          if (audio_device && fetcher.clock.is_playing()) audio_device->stop();
          {
            std::lock_guard<std::mutex> alter_lock(fetcher.alter_mutex);
            std::lock_guard<std::mutex> buffer_lock(fetcher.audio_buffer_mutex);
            fetcher.jump_to_time(clamp(requested_jump_time, 0.0, fetcher.get_duration()), system_clock_sec());
          }
          if (audio_device && fetcher.clock.is_playing()) audio_device->start();
        }

        // render_movie_screen(frame, vom, files[current_file]);

        if (COLS <= 20 || LINES < 10 || fullscreen) {
          print_pixel_data(frame, 0, 0, COLS, LINES, vom);
        } else {

          switch (fetcher.media_type) {
            case MediaType::VIDEO:
            case MediaType::AUDIO: {
              print_pixel_data(frame, 2, 0, COLS, LINES - 4, vom); // frame

              if (files.size() == 1) {
                wfill_box(stdscr, 1, 0, COLS, 1, '~');
                mvwaddstr_center(stdscr, 0, 0, COLS, "(" + std::to_string(current_file + 1) + "/" + std::to_string(files.size()) + ") " + std::filesystem::path(files[current_file]).filename().c_str());
              } else if (files.size() > 1) {
                wfill_box(stdscr, 2, 0, COLS, 1, '~');
                mvwaddstr_center(stdscr, 0, 0, COLS, "(" + std::to_string(current_file + 1) + "/" + std::to_string(files.size()) + ") " + std::filesystem::path(files[current_file]).filename().c_str());
                int rewind_file = playback_get_previous(current_file, files.size(), loop_type);
                int skip_file = playback_get_next(current_file, files.size(), loop_type);

                if (rewind_file >= 0) {
                  werasebox(stdscr, 1, 0, COLS / 2, 1);
                  mvwaddstr_left(stdscr, 1, 0, COLS / 2, "< " + std::filesystem::path(files[rewind_file]).filename().string());
                }

                if (skip_file >= 0) {
                  werasebox(stdscr, 1, COLS / 2, COLS / 2, 1);
                  mvwaddstr_right(stdscr, 1, COLS / 2, COLS / 2, std::filesystem::path(files[skip_file]).filename().string() + " >");
                }
              }

              wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
              wprint_playback_bar(stdscr, LINES - 2, 0, COLS, timestamp, fetcher.get_duration());
              const std::string loop_str = str_capslock(loop_type_to_string(loop_type)); 
              const std::string playing_str = fetcher.clock.is_playing() ? "PLAYING" : "PAUSED";
              const std::string volume_str = "VOLUME: " +  std::to_string((int)(volume * 100)) + "%";

              int section_size = COLS / 3;
              werasebox(stdscr, LINES - 1, 0, COLS, 1);
              mvwaddstr_center(stdscr, LINES - 1, 0, section_size, playing_str.c_str());
              mvwaddstr_center(stdscr, LINES - 1, section_size, section_size, loop_str.c_str());
              mvwaddstr_center(stdscr, LINES - 1, section_size * 2, section_size, volume_str.c_str());
            } break;
            case MediaType::IMAGE: {
              print_pixel_data(frame, 2, 0, COLS, LINES, vom); // frame

              if (files.size() == 1) {
                wfill_box(stdscr, 1, 0, COLS, 1, '~');
                mvwaddstr_center(stdscr, 0, 0, COLS, "(" + std::to_string(current_file + 1) + "/" + std::to_string(files.size()) + ") " + std::filesystem::path(files[current_file]).filename().c_str());
              } else if (files.size() > 1) {
                wfill_box(stdscr, 2, 0, COLS, 1, '~');
                mvwaddstr_center(stdscr, 0, 0, COLS, "(" + std::to_string(current_file + 1) + "/" + std::to_string(files.size()) + ") " + std::filesystem::path(files[current_file]).filename().c_str());
                int rewind_file = playback_get_previous(current_file, files.size(), loop_type);
                int skip_file = playback_get_next(current_file, files.size(), loop_type);

                if (rewind_file >= 0) {
                  werasebox(stdscr, 1, 0, COLS / 2, 1);
                  mvwaddstr_left(stdscr, 1, 0, COLS / 2, "< " + std::filesystem::path(files[rewind_file]).filename().string());
                }

                if (skip_file >= 0) {
                  werasebox(stdscr, 1, COLS / 2, COLS / 2, 1);
                  mvwaddstr_right(stdscr, 1, COLS / 2, COLS / 2, std::filesystem::path(files[skip_file]).filename().string() + " >");
                }
              }

            } break;
          }
          
        }

        refresh();
        sleep_for_ms(RENDER_LOOP_SLEEP_TIME_MS);
      }
    } catch (const std::exception& e) {
      std::lock_guard<std::mutex> lock(fetcher.alter_mutex);
      fetcher.in_use = false; // to tell other threads to end on error
      fetcher.error = std::string(e.what()); // to be caught after all threads are joined
    }

    if (audio_device) {
      audio_device->stop();
    }

    fetcher.join();

    if (fetcher.error.length() > 0) {
      ncurses_uninit();
      std::cerr << "[ascii_video]: Media Fetcher Error: " << fetcher.error << std::endl;
      return EXIT_FAILURE;
    }

    erase();
    current_file = next_file;
  } // !INTERRUPT_RECEIVED && current_file < files.size()

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
void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
  MediaFetcher* fetcher = (MediaFetcher*)(pDevice->pUserData);
  std::lock_guard<std::mutex> mutex_lock_guard(fetcher->audio_buffer_mutex);

  if (!fetcher->clock.is_playing() || !fetcher->audio_buffer->can_read(frameCount)) {
    float* pFloatOutput = (float*)pOutput;
    for (ma_uint32 i = 0; i < frameCount * pDevice->playback.channels; i++) {
      pFloatOutput[i] = 0.0001 * sin(0.10 * i);
    }
  } else if (fetcher->audio_buffer->can_read(frameCount)) {
    fetcher->audio_buffer->read_into(frameCount, (float*)pOutput);
  }

  (void)pInput;
}

void wprint_progress_bar(WINDOW* window, int y, int x, int width, int height, double percentage) {
  int passed_width = width * percentage;
  int remaining_width = width * (1.0 - percentage);
  wfill_box(window, y, x, passed_width, height, '@');
  wfill_box(window, y, x + passed_width, remaining_width, height, '-');
}

void wprint_playback_bar(WINDOW* window, int y, int x, int width, double time_in_seconds, double duration_in_seconds) {
  int PADDING_BETWEEN_ELEMENTS = 2;

  std::string formatted_passed_time = format_duration(time_in_seconds);
  std::string formatted_duration = format_duration(duration_in_seconds);
  std::string current_time_string = formatted_passed_time + " / " + formatted_duration;
  mvwprintw_left(window, y, x, width, current_time_string.c_str());

  int progress_bar_width = width - current_time_string.length() - PADDING_BETWEEN_ELEMENTS;

  if (progress_bar_width > 0) 
    wprint_progress_bar(window, y, x + current_time_string.length() + PADDING_BETWEEN_ELEMENTS, progress_bar_width, 1,time_in_seconds / duration_in_seconds);
}

void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode) {
  if (output_mode != VideoOutputMode::TEXT_ONLY && !has_colors()) {
    throw std::runtime_error("Attempted to print colored text in terminal that does not support color");
  }

  ScalingAlgo scaling_algorithm = bounds_width * bounds_height < (150 * 50) ? ScalingAlgo::BOX_SAMPLING : ScalingAlgo::NEAREST_NEIGHBOR;
  PixelData bounded = pixel_data.bound(bounds_width, bounds_height, scaling_algorithm);
  int image_start_row = bounds_row + std::abs(bounded.get_height() - bounds_height) / 2;
  int image_start_col = bounds_col + std::abs(bounded.get_width() - bounds_width) / 2; 

  bool background_only = output_mode == VideoOutputMode::COLORED_BACKGROUND_ONLY || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;

  for (int row = 0; row < bounded.get_height(); row++) {
    for (int col = 0; col < bounded.get_width(); col++) {
      const RGBColor& target_color = bounded.at(row, col);
      const char target_char = background_only ? ' ' : get_char_from_rgb(AsciiImage::ASCII_STANDARD_CHAR_MAP, target_color);
      
      if (output_mode == VideoOutputMode::TEXT_ONLY) {
        mvaddch(image_start_row + row, image_start_col + col, target_char);
      } else {
        const int color_pair = get_closest_ncurses_color_pair(target_color);
        mvaddch(image_start_row + row, image_start_col + col, target_char | COLOR_PAIR(color_pair));
      }
    }
  }

}