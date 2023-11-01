#include "tmedia.h"

#include "mediafetcher.h"
#include "wminiaudio.h"
#include "pixeldata.h"
#include "tmcurses.h"
#include "signalstate.h"
#include "wtime.h"
#include "wmath.h"
#include "wtime.h"
#include "tmedia_string.h"
#include "ascii.h"
#include "sleep.h"
#include "formatting.h"
#include "tmedia_vom.h"
#include "tmedia_render.h"

#include <rigtorp/SPSCQueue.h>

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

static constexpr int KEY_ESCAPE = 27;
static constexpr double VOLUME_CHANGE_AMOUNT = 0.01;
static constexpr int MIN_RENDER_COLS = 2;
static constexpr int MIN_RENDER_LINES = 2; 
static constexpr int AUDIO_QUEUE_SIZE = 4096;

void set_global_video_output_mode(VideoOutputMode* current, VideoOutputMode next);
void init_global_video_output_mode(VideoOutputMode mode);

const std::string TMEDIA_CONTROLS_USAGE = "-------CONTROLS-----------\n"
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
  "- 'P' - Rewind to Previous Media File\n"
  "- 'R' - Fully Refresh the Screen\n";

void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

struct AudioCallbackData {
  rigtorp::SPSCQueue<float>* audio_queue;
  std::atomic<bool> muted;
  AudioCallbackData(rigtorp::SPSCQueue<float>* audio_queue, bool muted) : audio_queue(audio_queue), muted(muted) {}
};

void audio_queue_fill_thread_func(MediaFetcher* source_fetcher, rigtorp::SPSCQueue<float>* dest_queue) {
  static constexpr int SOURCE_READ_BUFFER_SIZE = 512; // must be less than size of dest_queue
  static constexpr int BUFFER_FULL_RETRY_WAIT_MS = 5;
  static constexpr int FETCHER_PAUSED_WAIT_FOR_MS = 500;
  const int nb_channels = source_fetcher->audio_buffer->get_nb_channels();
  const int intermediary_frames_size = SOURCE_READ_BUFFER_SIZE / nb_channels;
  const int dest_queue_frames_size = dest_queue->capacity() / nb_channels;
  float intermediary[SOURCE_READ_BUFFER_SIZE];

  while (!source_fetcher->should_exit()) {
    {
      std::unique_lock<std::mutex> resume_notify_lock(source_fetcher->resume_notify_mutex);
      while (!source_fetcher->is_playing() && !source_fetcher->should_exit()) {
        source_fetcher->resume_cond.wait_for(resume_notify_lock, std::chrono::milliseconds(FETCHER_PAUSED_WAIT_FOR_MS));
      }
    }

    {
      std::lock_guard<std::mutex> buffer_reading_lock(source_fetcher->audio_buffer_mutex);
      if (source_fetcher->audio_buffer->get_nb_can_read() < dest_queue_frames_size * 2) {
        std::lock_guard<std::mutex> audio_buffer_request_lock(source_fetcher->audio_buffer_request_mutex);
        source_fetcher->audio_buffer_cond.notify_one();
      }

      if (source_fetcher->audio_buffer->get_nb_can_read() < intermediary_frames_size) continue;
      source_fetcher->audio_buffer->read_into(intermediary_frames_size, intermediary);
    }

    for (int i = 0; i < SOURCE_READ_BUFFER_SIZE; i++) {
      while (!dest_queue->try_push(intermediary[i])) {
        sleep_for_ms(BUFFER_FULL_RETRY_WAIT_MS);
      };
    }
  }
}

void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  const AudioCallbackData* data = (AudioCallbackData*)(pDevice->pUserData);
  for (ma_uint32 i = 0; i < frameCount * pDevice->playback.channels; i++) {
    if (data->audio_queue->front() != nullptr) {
      if (!data->muted) ((float*)pOutput)[i] = *data->audio_queue->front();
      else ((float*)pOutput)[i] = 0.0001 * sin(i * 0.01); // just garbage, but keeps audio device "active"
      data->audio_queue->pop();
    } else {
      ((float*)pOutput)[i] = 0.0001 * sin(i * 0.01); // just garbage, but keeps audio device "active"
    }
  }

  (void)pInput;
}


int tmedia(TMediaProgramData tmpd) {

  ncurses_init();
  init_global_video_output_mode(tmpd.vom);

  bool full_exit = false;
  while (!INTERRUPT_RECEIVED && !full_exit) {
    erase();
    PlaylistMoveCommand current_move_cmd = PlaylistMoveCommand::NEXT;
    MediaFetcher fetcher(tmpd.playlist.current());
    fetcher.requested_frame_dims = VideoDimensions(std::max(COLS, MIN_RENDER_COLS), std::max(LINES, MIN_RENDER_LINES));

    std::unique_ptr<ma_device_w> audio_device;
    rigtorp::SPSCQueue<float> audio_queue(AUDIO_QUEUE_SIZE);
    AudioCallbackData audio_device_user_data(&audio_queue, tmpd.muted);
    std::thread audio_queue_fill_thread;

    fetcher.begin();

    if (fetcher.has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      ma_device_config config = ma_device_config_init(ma_device_type_playback);
      config.playback.format  = ma_format_f32;
      config.playback.channels = fetcher.media_decoder->get_nb_channels();
      config.sampleRate = fetcher.media_decoder->get_sample_rate();       
      config.dataCallback = audioDataCallback;   
      config.pUserData = (void*)(&audio_device_user_data);
      config.noPreSilencedOutputBuffer = MA_TRUE;
      config.noClip = MA_TRUE;
      config.periodSizeInFrames = 0;
      config.periodSizeInMilliseconds = 10;
      config.periods = 3;

      std::thread initialized_audio_queue_fill_thread(audio_queue_fill_thread_func, &fetcher, &audio_queue);
      audio_queue_fill_thread.swap(initialized_audio_queue_fill_thread);
      audio_device = std::make_unique<ma_device_w>(&config);
      audio_device->start();
      audio_device->set_volume(tmpd.volume);
    }
    

    try {
      while (!fetcher.should_exit()) { // never break without setting in_use to false
        PixelData frame;
        if (INTERRUPT_RECEIVED) {
          fetcher.dispatch_exit();
          break;
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
            if (COLS >= MIN_RENDER_COLS && LINES >= MIN_RENDER_LINES) {
              std::lock_guard<std::mutex> alter_lock(fetcher.alter_mutex);
              fetcher.requested_frame_dims = VideoDimensions(COLS, LINES);
            }
            erase();
          }

          if (input == 'r' || input == 'R') {
            erase();
          }

          if ((input == 'c' || input == 'C') && has_colors() && can_change_color()) { // Change from current video mode to colored version
            switch (tmpd.vom) {
              case VideoOutputMode::COLORED: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::GRAYSCALE: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::COLORED); break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::COLORED_BACKGROUND_ONLY); break;
              case VideoOutputMode::TEXT_ONLY: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::COLORED); break;
            }
          }

          if ((input == 'g' || input == 'G') && has_colors() && can_change_color()) {
            switch (tmpd.vom) {
              case VideoOutputMode::COLORED: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::GRAYSCALE); break;
              case VideoOutputMode::GRAYSCALE: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY); break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::TEXT_ONLY: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::GRAYSCALE); break;
            }
          }

          if ((input == 'b' || input == 'B') && has_colors() && can_change_color()) {
            switch (tmpd.vom) {
              case VideoOutputMode::COLORED: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::COLORED_BACKGROUND_ONLY); break;
              case VideoOutputMode::GRAYSCALE: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY); break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::COLORED); break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: set_global_video_output_mode(&tmpd.vom, VideoOutputMode::GRAYSCALE); break;
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
            tmpd.fullscreen = !tmpd.fullscreen;
          }

          if (input == KEY_UP && audio_device) {
            tmpd.volume = clamp(tmpd.volume + VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
            audio_device->set_volume(tmpd.volume);
          }

          if (input == KEY_DOWN && audio_device) {
            tmpd.volume = clamp(tmpd.volume - VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
            audio_device->set_volume(tmpd.volume);
          }

          if ((input == 'm' || input == 'M') && audio_device) {
            tmpd.muted = !tmpd.muted;
            audio_device_user_data.muted = tmpd.muted;
          }

          if ((input == 'l' || input == 'L') && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            switch (tmpd.playlist.loop_type()) {
              case LoopType::NO_LOOP: tmpd.playlist.set_loop_type(LoopType::REPEAT); break;
              case LoopType::REPEAT: tmpd.playlist.set_loop_type(LoopType::REPEAT_ONE); break;
              case LoopType::REPEAT_ONE: tmpd.playlist.set_loop_type(LoopType::NO_LOOP); break;
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

        if (COLS < MIN_RENDER_COLS || LINES < MIN_RENDER_LINES) {
          erase();
        } else if (COLS <= 20 || LINES < 10 || tmpd.fullscreen) {
          render_pixel_data(frame, 0, 0, COLS, LINES, tmpd.vom, tmpd.scaling_algorithm, tmpd.ascii_display_chars);
        } else {
          render_pixel_data(frame, 2, 0, COLS, LINES - 4, tmpd.vom, tmpd.scaling_algorithm, tmpd.ascii_display_chars);

          if (tmpd.playlist.size() == 1) {
            wfill_box(stdscr, 1, 0, COLS, 1, '~');
            mvwaddstr_center(stdscr, 0, 0, COLS, "(" + std::to_string(tmpd.playlist.index() + 1) + "/" + std::to_string(tmpd.playlist.size()) + ") " + to_filename(tmpd.playlist.current()));
          } else if (tmpd.playlist.size() > 1) {
            wfill_box(stdscr, 2, 0, COLS, 1, '~');
            mvwaddstr_center(stdscr, 0, 0, COLS, "(" + std::to_string(tmpd.playlist.index() + 1) + "/" + std::to_string(tmpd.playlist.size()) + ") " + to_filename(tmpd.playlist.current()));

            if (tmpd.playlist.can_move(PlaylistMoveCommand::REWIND)) {
              werasebox(stdscr, 1, 0, COLS / 2, 1);
              mvwaddstr_left(stdscr, 1, 0, COLS / 2, "< " + to_filename(tmpd.playlist.peek_move(PlaylistMoveCommand::REWIND)));
            }

            if (tmpd.playlist.can_move(PlaylistMoveCommand::SKIP)) {
              werasebox(stdscr, 1, COLS / 2, COLS / 2, 1);
              mvwaddstr_right(stdscr, 1, COLS / 2, COLS / 2, to_filename(tmpd.playlist.peek_move(PlaylistMoveCommand::SKIP)) + " >");
            }
          }

          if (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO) {
            wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
            wprint_playback_bar(stdscr, LINES - 2, 0, COLS, timestamp, fetcher.get_duration());

            std::vector<std::string> bottom_labels;
            const std::string playing_str = fetcher.is_playing() ? "PLAYING" : "PAUSED";
            const std::string loop_str = str_capslock(loop_type_str(tmpd.playlist.loop_type())); 
            const std::string volume_str = "VOLUME: " + (tmpd.muted ? "MUTED" : (std::to_string((int)(tmpd.volume * 100)) + "%"));

            bottom_labels.push_back(playing_str);
            bottom_labels.push_back(loop_str);
            if (audio_device) bottom_labels.push_back(volume_str);
            werasebox(stdscr, LINES - 1, 0, COLS, 1);
            wprint_labels(stdscr, bottom_labels, LINES - 1, 0, COLS);
          }
        }

        refresh();
        if (tmpd.render_loop_max_fps) {
          std::unique_lock<std::mutex> exit_notify_lock(fetcher.exit_notify_mutex);
          if (!fetcher.should_exit()) {
            fetcher.exit_cond.wait_for(exit_notify_lock,  seconds_to_chrono_nanoseconds(1 / static_cast<double>(tmpd.render_loop_max_fps.value())));
          }
        }
      }
    } catch (const std::exception& err) {
      std::lock_guard<std::mutex> lock(fetcher.alter_mutex);
      fetcher.dispatch_exit(err.what());
    }

    if (audio_device) audio_device->stop();
    while (audio_queue.front() != nullptr) audio_queue.pop(); //flush audio_queue (REALLY IMPORTANT!)
    if (audio_queue_fill_thread.joinable()) audio_queue_fill_thread.join();
    fetcher.join();

    if (fetcher.has_error()) {
      std::cerr << "[tmedia]: Media Fetcher Error: " << fetcher.get_error() << std::endl;
      ncurses_uninit();
      return EXIT_FAILURE;
    }

    //flush getch
    while (getch() != ERR) getch(); 


    if (!tmpd.playlist.can_move(current_move_cmd)) break;
    tmpd.playlist.move(current_move_cmd);
  }

  ncurses_uninit();
  return EXIT_SUCCESS;
}


void init_global_video_output_mode(VideoOutputMode mode) {
  switch (mode) {
    case VideoOutputMode::COLORED:
    case VideoOutputMode::COLORED_BACKGROUND_ONLY: ncurses_set_color_palette(TMNCursesColorPalette::RGB); break;
    case VideoOutputMode::GRAYSCALE:
    case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: ncurses_set_color_palette(TMNCursesColorPalette::GRAYSCALE); break;
    case VideoOutputMode::TEXT_ONLY: break;
  }
}

void set_global_video_output_mode(VideoOutputMode* current, VideoOutputMode next) {
  init_global_video_output_mode(next);
  *current = next;
}