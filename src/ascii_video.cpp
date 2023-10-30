#include "ascii_video.h"

#include "mediafetcher.h"
#include "wminiaudio.h"
#include "pixeldata.h"
#include "avcurses.h"
#include "signalstate.h"
#include "wtime.h"
#include "wmath.h"
#include "wtime.h"
#include "asv_string.h"
#include "ascii.h"
#include "sleep.h"
#include "formatting.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <cstddef>
#include <cstdio>
#include <optional>
#include <filesystem>
#include <cctype>
#include <stdexcept>
#include <mutex>
#include <atomic>

extern "C" {
  #include <curses.h>
  #include <miniaudio.h>
}

const std::string ASCII_VIDEO_CONTROLS_USAGE = "-------CONTROLS-----------\n"
  "Video and Audio Controls\n"
  "- Space - Play and Pause\n"
  "- Up Arrow - Increase Volume 1%\n"
  "- Down Arrow - Decrease Volume 1%\n"
  "- Left Arrow - Skip Backward 5 Seconds\n"
  "- Right Arrow - Skip Forward 5 Seconds\n"
  "- Escape or Backspace or 'q' - Quit Program\n"
  "- '0' - Restart Playback\n"
  "- '1' through '9' - Skip To n/10 of the Media's Duration\n"
  "- 'L' - Switch looping type of playback (between no loop, repeat, and repeat one)\n"
  "- 'M' - Mute/Unmute Audio\n"
  "Video, Audio, and Image Controls\n"
  "- 'C' - Display Color (on supported terminals)\n"
  "- 'G' - Display Grayscale (on supported terminals)\n"
  "- 'B' - Display no Characters (on supported terminals) (must be in color or grayscale mode)\n"
  "- 'N' - Skip to Next Media File\n"
  "- 'P' - Rewind to Previous Media File\n";

void print_pixel_data(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode, const ScalingAlgo scaling_algorithm, const std::string& ascii_char_map);
void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void wprint_progress_bar(WINDOW* window, int y, int x, int width, int height, double percentage);
void wprint_playback_bar(WINDOW* window, int y, int x, int width, double time, double duration);
void wprint_labels(WINDOW* window, std::vector<std::string>& labels, int y, int x, int width);
void set_global_video_output_mode(VideoOutputMode* current, VideoOutputMode next);
void init_global_video_output_mode(VideoOutputMode mode);
std::string to_filename(const std::string& path);

struct AudioCallbackData {
  MediaFetcher* fetcher;
  std::atomic<bool> muted;
  AudioCallbackData(MediaFetcher* fetcher, bool muted) : fetcher(fetcher), muted(muted) {}
};

int ascii_video(AsciiVideoProgramData avpd) {
  const int KEY_ESCAPE = 27;
  const double VOLUME_CHANGE_AMOUNT = 0.01;
  ncurses_init();
  init_global_video_output_mode(avpd.vom);

  bool full_exit = false;
  while (!INTERRUPT_RECEIVED && !full_exit) {
    erase();
    PlaylistMoveCommand current_move_cmd = PlaylistMoveCommand::NEXT;
    MediaFetcher fetcher(avpd.playlist.current());
    std::unique_ptr<ma_device_w> audio_device;

    AudioCallbackData audio_device_user_data(&fetcher, avpd.muted);
    if (fetcher.has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      ma_device_config config = ma_device_config_init(ma_device_type_playback);
      config.playback.format  = ma_format_f32;
      config.playback.channels = fetcher.media_decoder->get_nb_channels();
      config.sampleRate = fetcher.media_decoder->get_sample_rate();       
      config.dataCallback = audioDataCallback;   
      config.pUserData = (void*)(&audio_device_user_data);

      audio_device = std::make_unique<ma_device_w>(&config);
      audio_device->start();
      audio_device->set_volume(avpd.volume);
    }

    {
      std::lock_guard<std::mutex> audio_buffer_lock(fetcher.audio_buffer_mutex);
      fetcher.begin();
    }

    try {
      while (!fetcher.should_exit()) { // never break without setting in_use to false
        PixelData frame;
        if (INTERRUPT_RECEIVED) {
          fetcher.dispatch_exit();
          break;
        }

        {
          std::lock_guard<std::mutex> alter_lock(fetcher.alter_mutex);
          fetcher.requested_frame_dims = VideoDimensions(COLS, LINES - 6);
        }

        double current_system_time = 0.0; // filler data to be filled in critical section
        double requested_jump_time = 0.0; // filler data to be filled in critical section
        double timestamp = 0.0; // filler data to be filled in critical section
        bool requested_jump = false;

        {
          std::lock_guard<std::mutex> _alter_lock(fetcher.alter_mutex);
          current_system_time = system_clock_sec(); // set in here, since locking the mutex could take an undetermined amount of time
          timestamp = fetcher.get_time(current_system_time);
          requested_jump_time = timestamp;
          frame = fetcher.frame;
        }

        int input = ERR;
        while ((input = getch()) != ERR) { // Go through and process all the batched input
          if (input == KEY_ESCAPE || input == KEY_BACKSPACE || input == 127 || input == '\b' || input == 'q' || input == 'Q') {
            fetcher.dispatch_exit();
            full_exit = true;
            break; // break out of input != ERR
          }

          if (input == KEY_RESIZE) {
            erase();
            refresh();
          }

          if ((input == 'c' || input == 'C') && has_colors() && can_change_color()) { // Change from current video mode to colored version
            switch (avpd.vom) {
              case VideoOutputMode::COLORED: set_global_video_output_mode(&avpd.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::GRAYSCALE: set_global_video_output_mode(&avpd.vom, VideoOutputMode::COLORED); break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY: set_global_video_output_mode(&avpd.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: set_global_video_output_mode(&avpd.vom, VideoOutputMode::COLORED_BACKGROUND_ONLY); break;
              case VideoOutputMode::TEXT_ONLY: set_global_video_output_mode(&avpd.vom, VideoOutputMode::COLORED); break;
            }
          }

          if ((input == 'g' || input == 'G') && has_colors() && can_change_color()) {
            switch (avpd.vom) {
              case VideoOutputMode::COLORED: set_global_video_output_mode(&avpd.vom, VideoOutputMode::GRAYSCALE); break;
              case VideoOutputMode::GRAYSCALE: set_global_video_output_mode(&avpd.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY: set_global_video_output_mode(&avpd.vom, VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY); break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: set_global_video_output_mode(&avpd.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::TEXT_ONLY: set_global_video_output_mode(&avpd.vom, VideoOutputMode::GRAYSCALE); break;
            }
          }

          if ((input == 'b' || input == 'B') && has_colors() && can_change_color()) {
            switch (avpd.vom) {
              case VideoOutputMode::COLORED: set_global_video_output_mode(&avpd.vom, VideoOutputMode::COLORED_BACKGROUND_ONLY); break;
              case VideoOutputMode::GRAYSCALE: set_global_video_output_mode(&avpd.vom, VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY); break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY: set_global_video_output_mode(&avpd.vom, VideoOutputMode::COLORED); break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: set_global_video_output_mode(&avpd.vom, VideoOutputMode::GRAYSCALE); break;
              case VideoOutputMode::TEXT_ONLY: break; //no-op
            }
          }

          if (input == 'n' || input == 'N') {
            current_move_cmd = PlaylistMoveCommand::SKIP;
            fetcher.dispatch_exit();
          }

          if (input == 'p' || input == 'P') {
            current_move_cmd = PlaylistMoveCommand::REWIND;
            fetcher.dispatch_exit();
          }

          if (input == 'f' || input == 'F') {
            erase();
            avpd.fullscreen = !avpd.fullscreen;
          }

          if (input == KEY_UP && audio_device) {
            avpd.volume = clamp(avpd.volume + VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
            audio_device->set_volume(avpd.volume);
          }

          if (input == KEY_DOWN && audio_device) {
            avpd.volume = clamp(avpd.volume - VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
            audio_device->set_volume(avpd.volume);
          }

          if ((input == 'm' || input == 'M') && audio_device) {
            avpd.muted = !avpd.muted;
            audio_device_user_data.muted = avpd.muted;
          }

          if ((input == 'l' || input == 'L') && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            switch (avpd.playlist.loop_type()) {
              case LoopType::NO_LOOP: avpd.playlist.set_loop_type(LoopType::REPEAT); break;
              case LoopType::REPEAT: avpd.playlist.set_loop_type(LoopType::REPEAT_ONE); break;
              case LoopType::REPEAT_ONE: avpd.playlist.set_loop_type(LoopType::NO_LOOP); break;
            }
          }

          if (input == ' ' && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            std::lock_guard<std::mutex> alter_lock(fetcher.alter_mutex); 
            switch (fetcher.is_playing()) {
              case true:  {
                if (audio_device) audio_device->stop();
                fetcher.pause(current_system_time);
              } break;
              case false: {
                if (audio_device) audio_device->start();
                fetcher.resume(current_system_time);
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

          if (std::isdigit(input) && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            requested_jump = true;
            requested_jump_time = fetcher.get_duration() * (static_cast<double>(input - static_cast<int>('0')) / 10.0);
          }
        } // Ending of "while (input != ERR)"

        if (requested_jump) {
          if (audio_device && fetcher.is_playing()) audio_device->stop();
          {
            std::scoped_lock<std::mutex, std::mutex> total_lock{fetcher.alter_mutex, fetcher.audio_buffer_mutex};
            fetcher.jump_to_time(clamp(requested_jump_time, 0.0, fetcher.get_duration()), system_clock_sec());
          }
          if (audio_device && fetcher.is_playing()) audio_device->start();
        }

        if (COLS <= 20 || LINES < 10 || avpd.fullscreen) {
          print_pixel_data(frame, 0, 0, COLS, LINES, avpd.vom, avpd.scaling_algorithm, avpd.ascii_display_chars);
        } else {
          print_pixel_data(frame, 2, 0, COLS, LINES - 4, avpd.vom, avpd.scaling_algorithm, avpd.ascii_display_chars);

          if (avpd.playlist.size() == 1) {
            wfill_box(stdscr, 1, 0, COLS, 1, '~');
            mvwaddstr_center(stdscr, 0, 0, COLS, "(" + std::to_string(avpd.playlist.index() + 1) + "/" + std::to_string(avpd.playlist.size()) + ") " + to_filename(avpd.playlist.current()));
          } else if (avpd.playlist.size() > 1) {
            wfill_box(stdscr, 2, 0, COLS, 1, '~');
            mvwaddstr_center(stdscr, 0, 0, COLS, "(" + std::to_string(avpd.playlist.index() + 1) + "/" + std::to_string(avpd.playlist.size()) + ") " + to_filename(avpd.playlist.current()));

            if (avpd.playlist.can_move(PlaylistMoveCommand::REWIND)) {
              werasebox(stdscr, 1, 0, COLS / 2, 1);
              mvwaddstr_left(stdscr, 1, 0, COLS / 2, "< " + to_filename(avpd.playlist.peek_move(PlaylistMoveCommand::REWIND)));
            }

            if (avpd.playlist.can_move(PlaylistMoveCommand::SKIP)) {
              werasebox(stdscr, 1, COLS / 2, COLS / 2, 1);
              mvwaddstr_right(stdscr, 1, COLS / 2, COLS / 2, to_filename(avpd.playlist.peek_move(PlaylistMoveCommand::SKIP)) + " >");
            }
          }

          if (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO) {
            wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
            wprint_playback_bar(stdscr, LINES - 2, 0, COLS, timestamp, fetcher.get_duration());

            std::vector<std::string> bottom_labels;
            const std::string playing_str = fetcher.is_playing() ? "PLAYING" : "PAUSED";
            const std::string loop_str = str_capslock(loop_type_str(avpd.playlist.loop_type())); 
            const std::string volume_str = "VOLUME: " + (avpd.muted ? "MUTED" : (std::to_string((int)(avpd.volume * 100)) + "%"));

            bottom_labels.push_back(playing_str);
            bottom_labels.push_back(loop_str);
            if (audio_device) bottom_labels.push_back(volume_str);
            werasebox(stdscr, LINES - 1, 0, COLS, 1);
            wprint_labels(stdscr, bottom_labels, LINES - 1, 0, COLS);
          }
        }

        refresh();
        if (avpd.render_loop_max_fps) {
          std::unique_lock<std::mutex> exit_notify_lock(fetcher.exit_notify_mutex);
          if (!fetcher.should_exit()) {
            fetcher.exit_cond.wait_for(exit_notify_lock,  seconds_to_chrono_nanoseconds(1 / static_cast<double>(avpd.render_loop_max_fps.value())));
          }
        }
      }
    } catch (const std::exception& err) {
      std::lock_guard<std::mutex> lock(fetcher.alter_mutex);
      fetcher.dispatch_exit(err.what());
    }

    if (audio_device) audio_device->stop();
    fetcher.join();

    if (fetcher.has_error()) {
      std::cerr << "[ascii_video]: Media Fetcher Error: " << fetcher.get_error() << std::endl;
      ncurses_uninit();
      return EXIT_FAILURE;
    }

    //flush getch
    while (getch() != ERR) getch(); 

    if (!avpd.playlist.can_move(current_move_cmd)) break;
    avpd.playlist.move(current_move_cmd);
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
void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
  const AudioCallbackData* data = (AudioCallbackData*)(pDevice->pUserData);
  MediaFetcher* fetcher = data->fetcher;
  std::lock_guard<std::mutex> mutex_lock_guard(fetcher->audio_buffer_mutex);
  bool muted = data->muted;

  if (!fetcher->is_playing() || !fetcher->audio_buffer->can_read(frameCount)) {
    for (ma_uint32 i = 0; i < frameCount * pDevice->playback.channels; i++) {
      ((float*)pOutput)[i] = 0.000001 * sin(0.10 * i);
    }
  } else if (fetcher->audio_buffer->can_read(frameCount)) {
    if (muted) {
      for (ma_uint32 i = 0; i < frameCount * pDevice->playback.channels; i++) {
        ((float*)pOutput)[i] = 0.000001 * sin(0.10 * i);
      }
      fetcher->audio_buffer->advance(frameCount);
    } else {
      fetcher->audio_buffer->read_into(frameCount, (float*)pOutput);
    }
  }

  if (!fetcher->audio_buffer->can_read(pDevice->sampleRate * 5)) {
    std::lock_guard<std::mutex> audio_buffer_request_lock(fetcher->audio_buffer_request_mutex);
    fetcher->audio_buffer_cond.notify_one();
  }

  (void)pInput;
}

std::string to_filename(const std::string& path_str) {
  return std::filesystem::path(path_str).filename().string();
}

void init_global_video_output_mode(VideoOutputMode mode) {
  switch (mode) {
    case VideoOutputMode::COLORED:
    case VideoOutputMode::COLORED_BACKGROUND_ONLY: ncurses_set_color_palette(AVNCursesColorPalette::RGB); break;
    case VideoOutputMode::GRAYSCALE:
    case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: ncurses_set_color_palette(AVNCursesColorPalette::GRAYSCALE); break;
    case VideoOutputMode::TEXT_ONLY: break;
  }
}

void set_global_video_output_mode(VideoOutputMode* current, VideoOutputMode next) {
  init_global_video_output_mode(next);
  *current = next;
}

void wprint_progress_bar(WINDOW* window, int y, int x, int width, int height, double percentage) {
  const int passed_width = width * percentage;
  const int remaining_width = width * (1.0 - percentage);
  wfill_box(window, y, x, passed_width, height, '@');
  wfill_box(window, y, x + passed_width, remaining_width, height, '-');
}

void wprint_playback_bar(WINDOW* window, int y, int x, int width, double time_in_seconds, double duration_in_seconds) {
  constexpr int PADDING_BETWEEN_ELEMENTS = 2;

  const std::string formatted_passed_time = format_duration(time_in_seconds);
  const std::string formatted_duration = format_duration(duration_in_seconds);
  const std::string current_time_string = formatted_passed_time + " / " + formatted_duration;
  mvwprintw_left(window, y, x, width, current_time_string.c_str());

  const int progress_bar_width = width - current_time_string.length() - PADDING_BETWEEN_ELEMENTS;

  if (progress_bar_width > 0) 
    wprint_progress_bar(window, y, x + current_time_string.length() + PADDING_BETWEEN_ELEMENTS, progress_bar_width, 1,time_in_seconds / duration_in_seconds);
}

void print_pixel_data(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VideoOutputMode output_mode, const ScalingAlgo scaling_algorithm, const std::string& ascii_char_map) {
  if (output_mode != VideoOutputMode::TEXT_ONLY && !has_colors()) {
    throw std::runtime_error("Attempted to print colored text in terminal that does not support color");
  }

  PixelData bounded = pixel_data.bound(bounds_width, bounds_height, scaling_algorithm);
  int image_start_row = bounds_row + std::abs(bounded.get_height() - bounds_height) / 2;
  int image_start_col = bounds_col + std::abs(bounded.get_width() - bounds_width) / 2; 
  bool background_only = output_mode == VideoOutputMode::COLORED_BACKGROUND_ONLY || output_mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;

  for (int row = 0; row < bounded.get_height(); row++) {
    for (int col = 0; col < bounded.get_width(); col++) {
      const RGBColor& target_color = bounded.at(row, col);
      const char target_char = background_only ? ' ' : get_char_from_rgb(ascii_char_map, target_color);
      
      if (output_mode == VideoOutputMode::TEXT_ONLY) {
        mvaddch(image_start_row + row, image_start_col + col, target_char);
      } else {
        const int color_pair = get_closest_ncurses_color_pair(target_color);
        mvaddch(image_start_row + row, image_start_col + col, target_char | COLOR_PAIR(color_pair));
      }
    }
  }

}

void wprint_labels(WINDOW* window, std::vector<std::string>& labels, int y, int x, int width) {
  if (width < 0)
    throw std::runtime_error("[wprint_labels] attempted to print to negative width space");
  if (width == 0)
    return;

  int section_size = width / labels.size();
  for (std::size_t i = 0; i < labels.size(); i++) {
    mvwaddstr_center(window, y, x + section_size * i, section_size, labels[i].c_str());
  }
}