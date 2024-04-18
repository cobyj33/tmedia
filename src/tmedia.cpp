#include <tmedia/tmedia.h>

#include <tmedia/media/mediafetcher.h>
#include <tmedia/audio/wminiaudio.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/tmcurses/tmcurses.h>
#include <tmedia/signalstate.h>
#include <tmedia/util/wtime.h>
#include <tmedia/util/wmath.h>
#include <tmedia/util/wtime.h>
#include <tmedia/util/defines.h>
#include <tmedia/util/sleep.h>
#include <tmedia/util/formatting.h>
#include <tmedia/tmedia_tui_elems.h>
#include <tmedia/audio/maaudioout.h>
#include <tmedia/image/palette.h>
#include <tmedia/util/defines.h>

#include <readerwritercircularbuffer.h>
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


TMediaProgramState tmss_to_tmps(TMediaStartupState& tmss) {
  TMediaProgramState tmps;
  tmps.ascii_display_chars = tmss.ascii_display_chars;
  tmps.fullscreen = tmss.fullscreen;
  tmps.muted = false;
  tmps.plist = Playlist(tmss.media_files, tmss.loop_type);
  if (tmss.shuffled) tmps.plist.shuffle(false);
  tmps.refresh_rate_fps = tmss.refresh_rate_fps;
  tmps.scaling_algorithm = tmss.scaling_algorithm;
  tmps.volume = tmss.volume;
  tmps.vom = tmss.vom;
  tmps.quit = false;
  tmps.req_frame_dim = Dim2(100, 100);
  return tmps;
}

int tmedia_main_loop(TMediaProgramState tmps);

int tmedia_run(TMediaStartupState& tmss) {
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
  tmps.req_frame_dim = Dim2(COLS, LINES);

  while (!INTERRUPT_RECEIVED && !tmps.quit && tmps.plist.size() > 0) {
    PlaylistMvCmd move_cmd = PlaylistMvCmd::NEXT;
    std::unique_ptr<MediaFetcher> fetcher;
    std::string currently_playing = tmps.plist.current();
    std::string cmd_buf; // currently unused

    try {
      const std::set<enum AVMediaType> streams = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
      fetcher = std::make_unique<MediaFetcher>(tmps.plist.current(), streams);
    } catch (const std::runtime_error& err) {
      std::size_t failed_plist_index = tmps.plist.index();

      // force skip this current file. If the loop type was in repeat-one, then
      // we might repeat it once we call move
      if (move_cmd == PlaylistMvCmd::NEXT) move_cmd = PlaylistMvCmd::SKIP;
      if (!tmps.plist.can_move(move_cmd)) break; // If we cannot move, we should just exit gracefully
      tmps.plist.move(move_cmd);

      tmps.plist.remove(failed_plist_index);
      continue;
    }


    fetcher->req_dims = Dim2(std::max(COLS, MIN_RENDER_COLS), std::max(LINES, MIN_RENDER_LINES));
    std::unique_ptr<MAAudioOut> audio_output;
    fetcher->begin(sys_clk_sec());

    if (fetcher->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      static constexpr int AUDIO_BUFFER_TRY_READ_MS = 5;
      audio_output = std::make_unique<MAAudioOut>(fetcher->mdec->get_nb_channels(), fetcher->mdec->get_sample_rate(), [&fetcher] (float* float_buffer, int nb_frames) {
        bool success = fetcher->audio_buffer->try_read_into(nb_frames, float_buffer, AUDIO_BUFFER_TRY_READ_MS);
        if (!success)
          for (int i = 0; i < nb_frames * fetcher->mdec->get_nb_channels(); i++)
            float_buffer[i] = 0.0f;
      });

      audio_output->set_volume(tmps.volume);
      audio_output->set_muted(tmps.muted);
      audio_output->start();
    }
    

    try {
      while (!fetcher->should_exit() && !INTERRUPT_RECEIVED) { // never break without using dispatch_exit on fetcher to false
        PixelData frame;
        double curr_systime, req_jumptime, curr_medtime;
        bool req_jump = false;

        {
          static constexpr double MAX_AUDIO_DESYNC_SECS = 0.6;  
          std::lock_guard<std::mutex> lock(fetcher->alter_mutex);
          curr_systime = sys_clk_sec(); // set in here, since locking the mutex could take an undetermined amount of time
          curr_medtime = fetcher->get_time(curr_systime);
          req_jumptime = curr_medtime;
          frame = fetcher->frame;
          fetcher->req_dims = tmps.req_frame_dim;
          req_jump = fetcher->get_desync_time(curr_systime) > MAX_AUDIO_DESYNC_SECS;
        }

        int input = ERR;
        while ((input = getch()) != ERR) { // Go through and process all the batched input
          switch (input) {
            case KEY_ESCAPE:
            case KEY_BACKSPACE:
            case 127:
            case '\b':
            case 'q':
            case 'Q': {
              fetcher->dispatch_exit();
              tmps.quit = true;
            } break;
            case KEY_RESIZE: {
              if (COLS >= MIN_RENDER_COLS && LINES >= MIN_RENDER_LINES) {
                std::lock_guard<std::mutex> alter_lock(fetcher->alter_mutex);
                fetcher->req_dims = Dim2(COLS, LINES);
              }
              erase();
            } break;
            case 'r':
            case 'R': {
              erase();
              if (audio_output && audio_output->playing()) {
                audio_output->stop();
                audio_output->start();
              } 
            } break;
            case 'c':
            case 'C': {
              switch (tmps.vom) {
                case VidOutMode::COLOR: set_global_vom(&tmps.vom, VidOutMode::PLAIN); break;
                case VidOutMode::GRAY: set_global_vom(&tmps.vom, VidOutMode::COLOR); break;
                case VidOutMode::COLOR_BG: set_global_vom(&tmps.vom, VidOutMode::PLAIN); break;
                case VidOutMode::GRAY_BG: set_global_vom(&tmps.vom, VidOutMode::COLOR_BG); break;
                case VidOutMode::PLAIN: set_global_vom(&tmps.vom, VidOutMode::COLOR); break;
              }
            } break;
            case 'g':
            case 'G': {
              switch (tmps.vom) {
                case VidOutMode::COLOR: set_global_vom(&tmps.vom, VidOutMode::GRAY); break;
                case VidOutMode::GRAY: set_global_vom(&tmps.vom, VidOutMode::PLAIN); break;
                case VidOutMode::COLOR_BG: set_global_vom(&tmps.vom, VidOutMode::GRAY_BG); break;
                case VidOutMode::GRAY_BG: set_global_vom(&tmps.vom, VidOutMode::PLAIN); break;
                case VidOutMode::PLAIN: set_global_vom(&tmps.vom, VidOutMode::GRAY); break;
              }
            } break;
            case 'b':
            case 'B': {
              if (has_colors() && can_change_color()) {
                switch (tmps.vom) {
                  case VidOutMode::COLOR: set_global_vom(&tmps.vom, VidOutMode::COLOR_BG); break;
                  case VidOutMode::GRAY: set_global_vom(&tmps.vom, VidOutMode::GRAY_BG); break;
                  case VidOutMode::COLOR_BG: set_global_vom(&tmps.vom, VidOutMode::COLOR); break;
                  case VidOutMode::GRAY_BG: set_global_vom(&tmps.vom, VidOutMode::GRAY); break;
                  case VidOutMode::PLAIN: break; //no-op
                }
              }
            } break;
            case 'n':
            case 'N': {
              move_cmd = PlaylistMvCmd::SKIP;
              fetcher->dispatch_exit();
            } break;
            case 'p':
            case 'P': {
              move_cmd = PlaylistMvCmd::REWIND;
              fetcher->dispatch_exit();
            } break;
            case 'f':
            case 'F': {
              erase();
              tmps.fullscreen = !tmps.fullscreen;
            } break;
            case KEY_UP: {
              if (audio_output) {
                tmps.volume = clamp(tmps.volume + VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
                audio_output->set_volume(tmps.volume);
              }
            } break;
            case KEY_DOWN: {
              if (audio_output) {
                tmps.volume = clamp(tmps.volume - VOLUME_CHANGE_AMOUNT, 0.0, 1.0);
                audio_output->set_volume(tmps.volume);
              }
            } break;
            case 'm':
            case 'M': {
              if (audio_output) {
                tmps.muted = !tmps.muted;
                audio_output->set_muted(tmps.muted);
              }
            } break;
            case 'l':
            case 'L': {
              if (fetcher->media_type == MediaType::VIDEO || fetcher->media_type == MediaType::AUDIO) {
                switch (tmps.plist.loop_type()) {
                  case LoopType::NO_LOOP: tmps.plist.set_loop_type(LoopType::REPEAT); break;
                  case LoopType::REPEAT: tmps.plist.set_loop_type(LoopType::REPEAT_ONE); break;
                  case LoopType::REPEAT_ONE: tmps.plist.set_loop_type(LoopType::NO_LOOP); break;
                }
              }
            } break;
            case 's':
            case 'S': {
              if (!tmps.plist.shuffled()) {
                tmps.plist.shuffle(true);
              } else {
                tmps.plist.unshuffle();
              }
            } break;
            case ' ': {
              if (fetcher->media_type == MediaType::VIDEO || fetcher->media_type == MediaType::AUDIO) {
                std::lock_guard<std::mutex> alter_lock(fetcher->alter_mutex); 
                switch (fetcher->is_playing()) {
                  case true:  {
                    if (audio_output) audio_output->stop();
                    fetcher->pause(curr_systime);
                  } break;
                  case false: {
                    if (audio_output) audio_output->start();
                    fetcher->resume(curr_systime);
                  } break;
                }
              }
            } break;
            case KEY_LEFT: {
              if (fetcher->media_type == MediaType::VIDEO || fetcher->media_type == MediaType::AUDIO) {
                req_jump = true;
                req_jumptime -= 5.0;
              }
            } break;
            case KEY_RIGHT: {
              if (fetcher->media_type == MediaType::VIDEO || fetcher->media_type == MediaType::AUDIO) {
                req_jump = true;
                req_jumptime += 5.0;
              }
            } break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
              if (fetcher->media_type == MediaType::VIDEO || fetcher->media_type == MediaType::AUDIO) {
                req_jump = true;
                req_jumptime = fetcher->get_duration() * (static_cast<double>(input - static_cast<int>('0')) / 10.0);
              }
            } break;
          }
        } // Ending of "while (input != ERR)"

        if (req_jump) {
          if (audio_output && fetcher->is_playing()) audio_output->stop();
          {
            std::scoped_lock<std::mutex> total_lock{fetcher->alter_mutex};
            fetcher->jump_to_time(clamp(req_jumptime, 0.0, fetcher->get_duration()), sys_clk_sec());
          }
          if (audio_output && fetcher->is_playing()) audio_output->start();
        }

        TMediaProgramSnapshot snapshot;
        snapshot.currently_playing = currently_playing;
        snapshot.frame = frame;
        snapshot.playing = fetcher->is_playing();
        snapshot.has_audio_output = audio_output ? true : false;
        snapshot.media_time_secs = curr_medtime;
        snapshot.media_duration_secs = fetcher->get_duration();
        snapshot.media_type = fetcher->media_type;

        renderer.render(tmps, snapshot);

        refresh();
        sleep_for_sec(1.0 / static_cast<double>(tmps.refresh_rate_fps));
      }
    } catch (const std::exception& err) {
      std::lock_guard<std::mutex> lock(fetcher->alter_mutex);
      fetcher->dispatch_exit(err.what());
    }

    fetcher->dispatch_exit();
    if (audio_output) audio_output->stop();
    fetcher->join(sys_clk_sec());
    if (fetcher->has_error()) {
      throw std::runtime_error(fmt::format("[{}]: Media Fetcher Error: {}",
      FUNCDINFO, fetcher->get_error()));
    }

    //flush getch
    while (getch() != ERR) getch(); 
    erase();
    if (!tmps.plist.can_move(move_cmd)) break;
    tmps.plist.move(move_cmd);
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