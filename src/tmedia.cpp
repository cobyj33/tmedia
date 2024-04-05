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
#include "funcmac.h"

#include "readerwritercircularbuffer.h"
#include <fmt/format.h>

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


void set_global_vom(VidOutMode* current, VidOutMode next);
void init_global_video_output_mode(VidOutMode mode);

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
  tmps.playlist = Playlist<std::filesystem::path>(tmss.media_files, tmss.loop_type);
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
    PlaylistMvCmd current_move_cmd = PlaylistMvCmd::NEXT;
    MediaFetcher fetcher(tmps.playlist.current());
    fetcher.req_dims = Dim2(std::max(COLS, MIN_RENDER_COLS), std::max(LINES, MIN_RENDER_LINES));
    std::unique_ptr<MAAudioOut> audio_output;
    fetcher.begin(sys_clk_sec());

    if (fetcher.has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      static constexpr int AUDIO_BUFFER_TRY_READ_MS = 5;
      audio_output = std::make_unique<MAAudioOut>(fetcher.mdec->get_nb_channels(), fetcher.mdec->get_sample_rate(), [&fetcher] (float* float_buffer, int nb_frames) {
        bool success = fetcher.audio_buffer->try_read_into(nb_frames, float_buffer, AUDIO_BUFFER_TRY_READ_MS);
        if (!success)
          for (int i = 0; i < nb_frames * fetcher.mdec->get_nb_channels(); i++)
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

        double curr_systime;
        double req_jumptime;
        double curr_medtime;
        bool req_jump = false;

        {
          static constexpr double MAX_AUDIO_DESYNC_SECS = 0.6;  
          std::lock_guard<std::mutex> _alter_lock(fetcher.alter_mutex);
          curr_systime = sys_clk_sec(); // set in here, since locking the mutex could take an undetermined amount of time
          curr_medtime = fetcher.get_time(curr_systime);
          req_jumptime = curr_medtime;
          frame = fetcher.frame;

          req_jump = fetcher.get_desync_time(curr_systime) > MAX_AUDIO_DESYNC_SECS;
        }

        int input = ERR;
        while ((input = getch()) != ERR) { // Go through and process all the batched input
          if (input == KEY_ESCAPE || input == KEY_BACKSPACE || input == 127 || 
              input == '\b' || input == 'q' || input == 'Q') {
            fetcher.dispatch_exit();
            tmps.quit = true;
            break; // break out of input != ERR
          }

          if (input == KEY_RESIZE) {
            if (COLS >= MIN_RENDER_COLS && LINES >= MIN_RENDER_LINES) {
              std::lock_guard<std::mutex> alter_lock(fetcher.alter_mutex);
              fetcher.req_dims = Dim2(COLS, LINES);
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
              case VidOutMode::COLOR: set_global_vom(&tmps.vom, VidOutMode::PLAIN); break;
              case VidOutMode::GRAY: set_global_vom(&tmps.vom, VidOutMode::COLOR); break;
              case VidOutMode::COLOR_BG: set_global_vom(&tmps.vom, VidOutMode::PLAIN); break;
              case VidOutMode::GRAY_BG: set_global_vom(&tmps.vom, VidOutMode::COLOR_BG); break;
              case VidOutMode::PLAIN: set_global_vom(&tmps.vom, VidOutMode::COLOR); break;
            }
          }

          if ((input == 'g' || input == 'G') && has_colors() && can_change_color()) {
            switch (tmps.vom) {
              case VidOutMode::COLOR: set_global_vom(&tmps.vom, VidOutMode::GRAY); break;
              case VidOutMode::GRAY: set_global_vom(&tmps.vom, VidOutMode::PLAIN); break;
              case VidOutMode::COLOR_BG: set_global_vom(&tmps.vom, VidOutMode::GRAY_BG); break;
              case VidOutMode::GRAY_BG: set_global_vom(&tmps.vom, VidOutMode::PLAIN); break;
              case VidOutMode::PLAIN: set_global_vom(&tmps.vom, VidOutMode::GRAY); break;
            }
          }

          if ((input == 'b' || input == 'B') && has_colors() && can_change_color()) {
            switch (tmps.vom) {
              case VidOutMode::COLOR: set_global_vom(&tmps.vom, VidOutMode::COLOR_BG); break;
              case VidOutMode::GRAY: set_global_vom(&tmps.vom, VidOutMode::GRAY_BG); break;
              case VidOutMode::COLOR_BG: set_global_vom(&tmps.vom, VidOutMode::COLOR); break;
              case VidOutMode::GRAY_BG: set_global_vom(&tmps.vom, VidOutMode::GRAY); break;
              case VidOutMode::PLAIN: break; //no-op
            }
          }

          if (input == 'n' || input == 'N') {
            current_move_cmd = PlaylistMvCmd::SKIP;
            fetcher.dispatch_exit();
          }

          if (input == 'p' || input == 'P') {
            current_move_cmd = PlaylistMvCmd::REWIND;
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
                fetcher.pause(curr_systime);
              } break;
              case false: {
                if (audio_output) audio_output->start();
                fetcher.resume(curr_systime);
              } break;
            }
          }

          if (input == KEY_LEFT && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            req_jump = true;
            req_jumptime -= 5.0;
          }

          if (input == KEY_RIGHT && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            req_jump = true;
            req_jumptime += 5.0;
          }

          if (std::isdigit(input) && (fetcher.media_type == MediaType::VIDEO || fetcher.media_type == MediaType::AUDIO)) {
            req_jump = true;
            req_jumptime = fetcher.get_duration() * (static_cast<double>(input - static_cast<int>('0')) / 10.0);
          }
        } // Ending of "while (input != ERR)"

        if (req_jump) {
          if (audio_output && fetcher.is_playing()) audio_output->stop();
          {
            std::scoped_lock<std::mutex> total_lock{fetcher.alter_mutex};
            fetcher.jump_to_time(clamp(req_jumptime, 0.0, fetcher.get_duration()), sys_clk_sec());
          }
          if (audio_output && fetcher.is_playing()) audio_output->start();
        }

        TMediaProgramSnapshot snapshot;
        snapshot.frame = frame;
        snapshot.playing = fetcher.media_type != MediaType::IMAGE ? fetcher.is_playing() : false;
        snapshot.has_audio_output = audio_output ? true : false;
        snapshot.media_time_secs = curr_medtime;
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

    fetcher.join(sys_clk_sec());
    if (fetcher.has_error()) {
      throw std::runtime_error(fmt::format("[{}]: Media Fetcher Error: {}",
      FUNCDINFO, fetcher.get_error()));
    }

    //flush getch
    while (getch() != ERR) getch(); 
    if (!tmps.playlist.can_move(current_move_cmd)) break;
    tmps.playlist.move(current_move_cmd);
  }

  return EXIT_SUCCESS;
}

void init_global_video_output_mode(VidOutMode mode) {
  switch (mode) {
    case VidOutMode::COLOR:
    case VidOutMode::COLOR_BG: ncurses_set_color_palette(TMNCursesColorPalette::RGB); break;
    case VidOutMode::GRAY:
    case VidOutMode::GRAY_BG: ncurses_set_color_palette(TMNCursesColorPalette::GRAYSCALE); break;
    case VidOutMode::PLAIN: break;
  }
}

void set_global_vom(VidOutMode* current, VidOutMode next) {
  init_global_video_output_mode(next);
  *current = next;
}