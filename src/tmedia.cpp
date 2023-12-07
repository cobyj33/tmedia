#include "tmedia.h"

#include "mediafetcher.h"
#include "wminiaudio.h"
#include "pixeldata.h"
#include "tmcurses.h"
#include "signalstate.h"
#include "wtime.h"
#include "wmath.h"
#include "wtime.h"
#include "ascii.h"
#include "sleep.h"
#include "formatting.h"
#include "tmedia_vom.h"
#include "tmedia_tui_elems.h"
#include "maaudioout.h"
#include "palette.h"

#include "readerwritercircularbuffer.h"

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
#include <algorithm>
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

TMediaProgramState tmss_to_tmps(TMediaStartupState tmss) {
  TMediaProgramState tmps;
  tmps.ascii_display_chars = tmss.ascii_display_chars;
  tmps.fullscreen = tmss.fullscreen;
  tmps.muted = false;
  tmps.playlist = Playlist(tmss.media_files, tmss.loop_type);
  if (tmss.shuffled) tmps.playlist.shuffle(false);
  tmps.refresh_rate_fps = tmss.refresh_rate_fps;
  tmps.scaling_algorithm = tmss.scaling_algorithm;
  tmps.volume = tmss.volume;
  tmps.vom = tmss.vom;
  tmps.quit = false;
  return tmps;
}

int tmedia_main_loop(TMediaProgramState tmps);

int tmedia_run(TMediaStartupState tmss) {
  ncurses_init();
  erase();
  TMediaProgramState tmps = tmss_to_tmps(tmss);
  init_global_video_output_mode(tmss.vom);
  int res = tmedia_main_loop(tmps);
  ncurses_uninit();
  return res;
}

int tmedia_main_loop(TMediaProgramState tmps) {
  TMediaCursesRenderer renderer;

  while (!INTERRUPT_RECEIVED && !tmps.quit) {
    PlaylistMoveCommand current_move_cmd = PlaylistMoveCommand::NEXT;
    MediaFetcher fetcher(tmps.playlist.current());
    fetcher.requested_frame_dims = VideoDimensions(std::max(COLS, MIN_RENDER_COLS), std::max(LINES, MIN_RENDER_LINES));
    std::unique_ptr<MAAudioOut> audio_output;
    fetcher.begin(system_clock_sec());

    if (fetcher.has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      static constexpr int AUDIO_BUFFER_TRY_READ_MS = 5;
      audio_output = std::make_unique<MAAudioOut>(fetcher.media_decoder->get_nb_channels(), fetcher.media_decoder->get_sample_rate(), [&fetcher] (float* float_buffer, int nb_frames) {
        bool success = fetcher.audio_buffer->try_read_into(nb_frames, float_buffer, AUDIO_BUFFER_TRY_READ_MS);
        if (!success)
          for (int i = 0; i < nb_frames * fetcher.media_decoder->get_nb_channels(); i++)
            float_buffer[i] = 0.0f;
      });

      audio_output->set_volume(tmps.volume);
      audio_output->set_muted(tmps.muted);
      audio_output->start();
    }
    

    try {
      while (!fetcher.should_exit()) { // never break without using dispatch_exit on fetcher to false
        PixelData frame;

        if (INTERRUPT_RECEIVED) {
          fetcher.dispatch_exit();
          break;
        }

        double current_system_time;
        double requested_jump_time;
        double current_media_time;
        bool requested_jump = false;

        {
          static constexpr double MAX_AUDIO_DESYNC_AMOUNT_SECONDS = 0.6;  
          std::lock_guard<std::mutex> _alter_lock(fetcher.alter_mutex);
          current_system_time = system_clock_sec(); // set in here, since locking the mutex could take an undetermined amount of time
          current_media_time = fetcher.get_time(current_system_time);
          requested_jump_time = current_media_time;
          frame = fetcher.frame;

          if (fetcher.get_desync_time(current_system_time) > MAX_AUDIO_DESYNC_AMOUNT_SECONDS) {
            requested_jump = true;
          }
        }

        int input = ERR;
        while ((input = getch()) != ERR) { // Go through and process all the batched input
          if (input == KEY_ESCAPE || input == KEY_BACKSPACE || input == 127 || input == '\b' || input == 'q' || input == 'Q') {
            fetcher.dispatch_exit();
            tmps.quit = true;
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
            if (audio_output && audio_output->playing()) {
              audio_output->stop();
              audio_output->start();
            }
          }

          if ((input == 'c' || input == 'C') && has_colors() && can_change_color()) { // Change from current video mode to colored version
            switch (tmps.vom) {
              case VideoOutputMode::COLORED: set_global_video_output_mode(&tmps.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::GRAYSCALE: set_global_video_output_mode(&tmps.vom, VideoOutputMode::COLORED); break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY: set_global_video_output_mode(&tmps.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: set_global_video_output_mode(&tmps.vom, VideoOutputMode::COLORED_BACKGROUND_ONLY); break;
              case VideoOutputMode::TEXT_ONLY: set_global_video_output_mode(&tmps.vom, VideoOutputMode::COLORED); break;
            }
          }

          if ((input == 'g' || input == 'G') && has_colors() && can_change_color()) {
            switch (tmps.vom) {
              case VideoOutputMode::COLORED: set_global_video_output_mode(&tmps.vom, VideoOutputMode::GRAYSCALE); break;
              case VideoOutputMode::GRAYSCALE: set_global_video_output_mode(&tmps.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY: set_global_video_output_mode(&tmps.vom, VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY); break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: set_global_video_output_mode(&tmps.vom, VideoOutputMode::TEXT_ONLY); break;
              case VideoOutputMode::TEXT_ONLY: set_global_video_output_mode(&tmps.vom, VideoOutputMode::GRAYSCALE); break;
            }
          }

          if ((input == 'b' || input == 'B') && has_colors() && can_change_color()) {
            switch (tmps.vom) {
              case VideoOutputMode::COLORED: set_global_video_output_mode(&tmps.vom, VideoOutputMode::COLORED_BACKGROUND_ONLY); break;
              case VideoOutputMode::GRAYSCALE: set_global_video_output_mode(&tmps.vom, VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY); break;
              case VideoOutputMode::COLORED_BACKGROUND_ONLY: set_global_video_output_mode(&tmps.vom, VideoOutputMode::COLORED); break;
              case VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY: set_global_video_output_mode(&tmps.vom, VideoOutputMode::GRAYSCALE); break;
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
            tmps.fullscreen = !tmps.fullscreen;
          }

          if (input == KEY_UP && audio_output) {
            tmps.volume = clamp(tmps.volume + VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
            audio_output->set_volume(tmps.volume);
          }

          if (input == KEY_DOWN && audio_output) {
            tmps.volume = clamp(tmps.volume - VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
            audio_output->set_volume(tmps.volume);
          }

          if ((input == 'm' || input == 'M') && audio_output) {
            tmps.muted = !tmps.muted;
            audio_output->set_muted(tmps.muted);
          }

          if ((input == 'l' || input == 'L') && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            switch (tmps.playlist.loop_type()) {
              case LoopType::NO_LOOP: tmps.playlist.set_loop_type(LoopType::REPEAT); break;
              case LoopType::REPEAT: tmps.playlist.set_loop_type(LoopType::REPEAT_ONE); break;
              case LoopType::REPEAT_ONE: tmps.playlist.set_loop_type(LoopType::NO_LOOP); break;
            }
          }

          if ((input == 's' || input == 'S') && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            if (!tmps.playlist.shuffled()) {
              tmps.playlist.shuffle(true);
            } else {
              tmps.playlist.unshuffle();
            }
          }

          if (input == ' ' && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            std::lock_guard<std::mutex> alter_lock(fetcher.alter_mutex); 
            switch (fetcher.is_playing()) {
              case true:  {
                if (audio_output) audio_output->stop();
                fetcher.pause(current_system_time);
              } break;
              case false: {
                if (audio_output) audio_output->start();
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
          if (audio_output && fetcher.is_playing()) audio_output->stop();
          {
            std::scoped_lock<std::mutex> total_lock{fetcher.alter_mutex};
            fetcher.jump_to_time(clamp(requested_jump_time, 0.0, fetcher.get_duration()), system_clock_sec());
          }
          if (audio_output && fetcher.is_playing()) audio_output->start();
        }

        TMediaProgramSnapshot snapshot;
        snapshot.frame = frame;
        snapshot.playing = fetcher.is_playing();
        snapshot.has_audio_output = audio_output ? true : false;
        snapshot.media_time_secs = current_media_time;
        snapshot.media_duration_secs = fetcher.get_duration();
        snapshot.media_type = fetcher.media_type;

        renderer.render(tmps, snapshot);

        refresh();
        sleep_for_sec(1.0 / static_cast<double>(tmps.refresh_rate_fps));
      }
    } catch (const std::exception& err) {
      std::lock_guard<std::mutex> lock(fetcher.alter_mutex);
      fetcher.dispatch_exit(err.what());
    }

    fetcher.join(system_clock_sec());
    if (fetcher.has_error()) {
      throw std::runtime_error("[tmedia]: Media Fetcher Error: " + fetcher.get_error());
    }

    //flush getch
    while (getch() != ERR) getch(); 
    if (!tmps.playlist.can_move(current_move_cmd)) break;
    tmps.playlist.move(current_move_cmd);
  }

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