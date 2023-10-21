#include "termenv.h"

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

void render_movie_screen(PixelData& pixel_data, VideoOutputMode media_gui);
void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode);
void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

bool INTERRUPT_RECEIVED = false;
std::atomic<unsigned int> TERM_LINES = 24;
std::atomic<unsigned int> TERM_COLS = 80;

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
  // LoopType loop_type = LoopType::NO_LOOP;
  
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
              "p - Go to previous media file\n");

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

  std::vector<std::string> paths = parser.get<std::vector<std::string>>("paths");
  bool colors = parser.get<bool>("-c");
  bool grayscale = !colors && parser.get<bool>("-g");
  bool background = parser.get<bool>("-b");
  bool dump = parser.get<bool>("-d");
  bool print_ffmpeg_version = parser.get<bool>("--ffmpeg-version");
  bool print_curses_version = parser.get<bool>("--curses-version");

  muted = parser.get<bool>("-m");
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
    std::size_t next_file = current_file + 1;
    
    // switch (loop_type) {
    //   case LoopType::NO_LOOP: next_file = current_file + 1; break;
    //   case LoopType::REPEAT: next_file = current_file + 1 > files.size() ? 0 : current_file + 1; break;
    //   case LoopType::REPEAT_ONE: next_file = current_file; break;
    //   default: throw std::runtime_error("[ascii_video] Unimplemented Looping Type");
    // }

    if (fetcher.has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      ma_device_config config = ma_device_config_init(ma_device_type_playback);
      config.playback.format  = ma_format_f32;
      config.playback.channels = fetcher.media_decoder->get_nb_channels();
      config.sampleRate = fetcher.media_decoder->get_sample_rate();       
      config.dataCallback = audioDataCallback;   
      config.pUserData = &fetcher;

      audio_device = std::make_unique<ma_device_w>(nullptr, &config);
      audio_device->start();
      audio_device->set_volume(volume);
    }

    fetcher.in_use = true;
    fetcher.clock.start(system_clock_sec());
    std::thread video_thread(&MediaFetcher::video_fetching_thread, &fetcher);
    std::thread audio_thread;
    bool audio_thread_initialized = false;
    if (fetcher.has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      std::thread initialized_audio_thread(&MediaFetcher::audio_fetching_thread, &fetcher);
      audio_thread.swap(initialized_audio_thread);
      audio_thread_initialized = true;
    }
    PixelData frame;
    
    try {
      while (fetcher.in_use) { // never break without setting in_use to false
        TERM_COLS = COLS;
        TERM_LINES = LINES;

        if (INTERRUPT_RECEIVED) {
          fetcher.in_use = false;
          break;
        }

        {
          std::lock_guard<std::mutex> _alter_lock(fetcher.alter_mutex);
          std::lock_guard<std::mutex> _buffer_lock(fetcher.buffer_read_mutex); //needed for jump_time
          const double current_system_time = system_clock_sec();
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

            if (input == 'n' || input == 'N') {
              fetcher.in_use = false;
            }

            if (input == 'p' || input == 'P') {
              next_file = current_file == 0 ? 0 : current_file - 1;
              fetcher.in_use = false;
            }

            if (input == KEY_UP && audio_device) {
              volume = clamp(volume + VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
              audio_device->set_volume(volume);
            }

            if (input == KEY_DOWN && audio_device) {
              volume = clamp(volume - VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
              audio_device->set_volume(volume);
            }

            if (input == KEY_LEFT && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(clamp(fetcher.get_time(current_system_time) - 5.0, 0.0, fetcher.get_duration()), current_system_time);
            }

            if (input == KEY_RIGHT && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(clamp(fetcher.get_time(current_system_time) + 5.0, 0.0, fetcher.get_duration()), current_system_time);
            }

            if (input == ' ' && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.clock.toggle(current_system_time);
            }

            if (input == 'm' || input == 'M') {
              muted = !muted;
            }

            if (input == '0'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(0.0, current_system_time);
            }

            if (input == '1'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(fetcher.get_duration() * 1.0 / 10.0, current_system_time);
            }

            if (input == '2'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(fetcher.get_duration() * 2.0 / 10.0, current_system_time);
            }

            if (input == '3'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(fetcher.get_duration() * 3.0 / 10.0, current_system_time);
            }

            if (input == '4'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(fetcher.get_duration() * 4.0 / 10.0, current_system_time);
            }

            if (input == '5'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(fetcher.get_duration() * 5.0 / 10.0, current_system_time);
            }

            if (input == '6'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(fetcher.get_duration() * 6.0 / 10.0, current_system_time);
            }

            if (input == '7'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(fetcher.get_duration() * 7.0 / 10.0, current_system_time);
            }

            if (input == '8'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(fetcher.get_duration() * 8.0 / 10.0, current_system_time);
            }

            if (input == '9'  && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
              fetcher.jump_to_time(fetcher.get_duration() * 9.0 / 10.0, current_system_time);
            }

            input = getch();
          } // Ending of "while (input != ERR)"

        }

        render_movie_screen(frame, vom);
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

    video_thread.join();
    if (audio_thread_initialized) {
      audio_thread.join();
    }

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
  std::lock_guard<std::mutex> mutex_lock_guard(fetcher->buffer_read_mutex);

  if (!fetcher->clock.is_playing() || !fetcher->audio_buffer->can_read(frameCount)) {
    float* pFloatOutput = (float*)pOutput;
    for (ma_uint32 i = 0; i < frameCount; i++) {
      pFloatOutput[i] = 0.0001 * sin(0.10 * i);
    }
  } else if (fetcher->audio_buffer->can_read(frameCount)) {
    fetcher->audio_buffer->read_into(frameCount, (float*)pOutput);
  }

  (void)pInput;
}

void render_movie_screen(PixelData& pixel_data, VideoOutputMode output_mode) {
  print_pixel_data(pixel_data, 0, 0, COLS, LINES, output_mode);
}

void print_pixel_data(PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode) {
  if (output_mode != VideoOutputMode::TEXT_ONLY && !has_colors()) {
    throw std::runtime_error("Attempted to print colored text in terminal that does not support color");
  }

  PixelData bounded = pixel_data.bound(bounds_width, bounds_height);
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