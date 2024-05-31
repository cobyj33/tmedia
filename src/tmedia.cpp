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

using namespace std::chrono_literals;

static constexpr int KEY_ESCAPE = 27;
static constexpr double VOLUME_CHANGE_AMOUNT = 0.01;
static constexpr int MIN_RENDER_COLS = 2;
static constexpr int MIN_RENDER_LINES = 2; 


// Modify the tmcurses color palette to align with the next video output mode
// and set the value at *current to the value of next*
void set_global_vom(VidOutMode* current, VidOutMode next);

// Modify the tmcurses color palette to align with the given video output mode
void init_global_video_output_mode(VidOutMode mode);


TMediaProgramState tmss_to_tmps(TMediaStartupState& tmss) {
  TMediaProgramState tmps;
  tmps.ascii_display_chars = tmss.ascii_display_chars;
  tmps.fullscreen = tmss.fullscreen;
  tmps.muted = tmss.muted;
  tmps.plist = Playlist(tmss.media_files, tmss.loop_type);
  if (tmss.shuffled) tmps.plist.shuffle(false);
  tmps.refresh_rate_fps = tmss.refresh_rate_fps;
  tmps.volume = tmss.volume;
  tmps.vom = tmss.vom;
  tmps.quit = false;
  return tmps;
}

int tmedia_main_loop(TMediaProgramState tmps);

int tmedia_run(TMediaStartupState& tmss) {
  tmcurses_init();
  erase();
  TMediaProgramState tmps = tmss_to_tmps(tmss);
  init_global_video_output_mode(tmss.vom);
  int res = tmedia_main_loop(tmps);
  tmcurses_uninit();
  return res;
}

int tmedia_main_loop(TMediaProgramState tmps) {
  TMediaRendererState tmrs;
  pixdata_setnewdims(tmrs.scaling_buffer, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
  tmrs.req_frame_dim = Dim2(COLS, LINES);

  PixelData frame;
  pixdata_initgray(frame, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, 0);

  while (!INTERRUPT_RECEIVED && !tmps.quit && tmps.plist.size() > 0) {
    pixdata_initgray(frame, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, 0);

    // main loop setup phase. Sets up and begins MediaFetcher. Sets up
    // and begins audio output (if applicable)
    PlaylistMvCmd move_cmd = PlaylistMvCmd::NEXT;
    std::unique_ptr<MediaFetcher> fetcher;
    const PlaylistItem& pcurr = tmps.plist.current();

    try {
      fetcher = std::make_unique<MediaFetcher>(pcurr.path, pcurr.requested_streams);
    } catch (const std::runtime_error& err) {
      const std::size_t failed_plist_index = tmps.plist.index();

      // force skip this current file. If we cannot move, we should just
      // exit gracefully
      if (!tmps.plist.can_move(PlaylistMvCmd::SKIP)) break;
      tmps.plist.move(PlaylistMvCmd::SKIP);
      tmps.plist.remove(failed_plist_index);
      continue;
    }


    fetcher->req_dims = Dim2(std::max(COLS, MIN_RENDER_COLS), std::max(LINES, MIN_RENDER_LINES));
    std::unique_ptr<MAAudioOut> audio_output;
    fetcher->begin(sys_clk_sec()); // begin the MediaFetcher before beginning the
    // audio playback. Some audio data should be available by then hopefully

    if (fetcher->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      static constexpr std::chrono::milliseconds AUDIO_BUFFER_TRY_READ_MS = 2ms;
      audio_output = std::make_unique<MAAudioOut>(fetcher->mdec->get_nb_channels(), fetcher->mdec->get_sample_rate(), [&fetcher] (float* float_buffer, int nb_frames) {
        const bool success = fetcher->audio_buffer->try_read_into(nb_frames, float_buffer, AUDIO_BUFFER_TRY_READ_MS);
        if (!success) {
          const int fb_sz = nb_frames * fetcher->mdec->get_nb_channels();
          for (int i = 0; i < fb_sz; i++)
            float_buffer[i] = 0.0f;
        }
      });

      audio_output->set_volume(tmps.volume);
      audio_output->set_muted(tmps.muted);
      audio_output->start();
    }
    

    try {
      // main playing loop
      while (!fetcher->should_exit() && !INTERRUPT_RECEIVED) { // never break without using dispatch_exit on fetcher to false
        double curr_systime, req_jumptime, curr_medtime;

        // req_jump and req_jumptime are different, because when jumping to the
        // current time to fix desyncing of audio playback to the media clock,
        // we may jump to the current media time. Therefore, we can't use the
        // current media time (curr_medtime) as a sort of sentinel value
        // for denoting if we're trying to jump toward a timestamp
        bool req_jump = false;

        {
          static constexpr double MAX_AUDIO_DESYNC_SECS = 0.6;  
          std::lock_guard<std::mutex> lock(fetcher->alter_mutex);
          curr_systime = sys_clk_sec(); // set in here, since locking the mutex could take an undetermined amount of time
          curr_medtime = fetcher->get_time(curr_systime);
          req_jumptime = curr_medtime;
          pixdata_copy(frame, fetcher->frame);
          fetcher->req_dims = tmrs.req_frame_dim;

          // If the audio is desynced, we just try and jump to the current media
          // time to resync it.
          req_jump = fetcher->get_audio_desync_time(curr_systime) > MAX_AUDIO_DESYNC_SECS;
        }

        int input = getch();
        if (input != ERR) { // only process input if there is any input
          bool toggle_playback = false;
          bool should_refresh = false;
          bool toggle_shuffled = false;
          bool toggle_fullscreen = false;
          bool toggle_muted = false;
          bool quit_program_command_received = false;
          bool exit_current_player_command_received = false;
          double toggled_volume = tmps.volume;
          VidOutMode nextvom = tmps.vom;
          
          while (input != ERR) { // Go through and process all the batched input
            switch (input) {
              case KEY_ESCAPE:
              case KEY_BACKSPACE:
              case 127:
              case '\b':
              case 'q':
              case 'Q': quit_program_command_received = true; break;
              case KEY_RESIZE: erase(); break;
              case 'r':
              case 'R': should_refresh = !should_refresh; break;
              case 'c':
              case 'C': {
                switch (nextvom) {
                  case VidOutMode::COLOR: nextvom = VidOutMode::PLAIN; break;
                  case VidOutMode::GRAY: nextvom = VidOutMode::COLOR; break;
                  case VidOutMode::COLOR_BG: nextvom = VidOutMode::PLAIN; break;
                  case VidOutMode::GRAY_BG: nextvom = VidOutMode::COLOR_BG; break;
                  case VidOutMode::PLAIN: nextvom = VidOutMode::COLOR; break;
                }
              } break;
              case 'g':
              case 'G': {
                switch (nextvom) {
                  case VidOutMode::COLOR: nextvom = VidOutMode::GRAY; break;
                  case VidOutMode::GRAY: nextvom = VidOutMode::PLAIN; break;
                  case VidOutMode::COLOR_BG: nextvom = VidOutMode::GRAY_BG; break;
                  case VidOutMode::GRAY_BG: nextvom = VidOutMode::PLAIN; break;
                  case VidOutMode::PLAIN: nextvom = VidOutMode::GRAY; break;
                }
              } break;
              case 'b':
              case 'B': {
                switch (nextvom) {
                  case VidOutMode::COLOR: nextvom = VidOutMode::COLOR_BG; break;
                  case VidOutMode::GRAY: nextvom = VidOutMode::GRAY_BG; break;
                  case VidOutMode::COLOR_BG: nextvom = VidOutMode::COLOR; break;
                  case VidOutMode::GRAY_BG: nextvom = VidOutMode::GRAY; break;
                  case VidOutMode::PLAIN: break; //no-op
                }
              } break;
              case 'n':
              case 'N': {
                move_cmd = PlaylistMvCmd::SKIP;
                exit_current_player_command_received = true;
              } break;
              case 'p':
              case 'P': {
                move_cmd = PlaylistMvCmd::REWIND;
                exit_current_player_command_received = true;
              } break;
              case 'f':
              case 'F': toggle_fullscreen = !toggle_fullscreen; break;
              case KEY_UP: toggled_volume += VOLUME_CHANGE_AMOUNT; break;
              case KEY_DOWN: toggled_volume -= VOLUME_CHANGE_AMOUNT; break;
              case 'm':
              case 'M': toggle_muted = !toggle_muted; break;
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
              case 'S': toggle_shuffled = !toggle_shuffled; break;
              case ' ': toggle_playback = !toggle_playback; break;
              case KEY_LEFT: {
                req_jump = true;
                req_jumptime -= 5.0;
              } break;
              case KEY_RIGHT: {
                req_jump = true;
                req_jumptime += 5.0;
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
                req_jump = true;
                req_jumptime = fetcher->get_duration() * (static_cast<double>(input - static_cast<int>('0')) / 10.0);
              } break;
            }
            input = getch();
          } // Ending of "while (input != ERR)"

          if (exit_current_player_command_received) {
            fetcher->dispatch_exit();
          }

          if (quit_program_command_received) {
            fetcher->dispatch_exit();
            tmps.quit = true;
          }

          if (nextvom != tmps.vom) {
            set_global_vom(&tmps.vom, nextvom);
          }

          if (toggle_fullscreen) {
            erase();
            tmps.fullscreen = !tmps.fullscreen;
          }

          if (should_refresh) {
            erase();
            if (audio_output && audio_output->playing()) {
              audio_output->stop();
              audio_output->start();
            }
          }

          if (toggle_shuffled) {
            if (!tmps.plist.shuffled()) {
                tmps.plist.shuffle(true);
              } else {
                tmps.plist.unshuffle();
              }
          }

          if (audio_output) {
            if (toggled_volume != tmps.volume) {
              tmps.volume = clamp(toggled_volume, 0.0, 1.0);
              audio_output->set_volume(tmps.volume);
            }

            if (toggle_muted) {
              tmps.muted = !tmps.muted;
              audio_output->set_muted(tmps.muted);
            }
          }

          if (toggle_playback && (fetcher->media_type == MediaType::VIDEO || fetcher->media_type == MediaType::AUDIO)) {
            std::lock_guard<std::mutex> alter_lock(fetcher->alter_mutex);
            if (fetcher->is_playing()) {
              if (audio_output) audio_output->stop();
              fetcher->pause(curr_systime);
            } else  {
              if (audio_output) audio_output->start();
              fetcher->resume(curr_systime);
            }
          }
        } // end of input processing block ( if (input != ERR) )

        // we can jump time even if no input is given, in the case that audio
        // has become desynced and a jump was requested to resync all streams.
        // therefore, this block has to stay outside of the input processing
        // block
        if (req_jump && (fetcher->media_type == MediaType::VIDEO || fetcher->media_type == MediaType::AUDIO)) {
          if (audio_output && fetcher->is_playing()) audio_output->stop();
          {
            std::scoped_lock<std::mutex> total_lock{fetcher->alter_mutex};
            fetcher->jump_to_time(clamp(req_jumptime, 0.0, fetcher->get_duration()), sys_clk_sec());
          }
          if (audio_output && fetcher->is_playing()) audio_output->start();
        }

        TMediaProgramSnapshot snapshot;
        snapshot.frame = &frame;
        snapshot.playing = fetcher->is_playing();
        snapshot.has_audio_output = audio_output.get() != nullptr;
        snapshot.media_time_secs = curr_medtime;
        snapshot.media_duration_secs = fetcher->get_duration();
        snapshot.media_type = fetcher->media_type;

        Dim2 req_frame_dims_before = tmrs.req_frame_dim;
        render_tui(tmps, snapshot, tmrs);
        if (req_frame_dims_before != tmrs.req_frame_dim) {
          std::lock_guard<std::mutex> alter_lock(fetcher->alter_mutex);
          fetcher->req_dims = Dim2(tmrs.req_frame_dim);
        }

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
  } // end of main loop

  return EXIT_SUCCESS;
}

void init_global_video_output_mode(VidOutMode mode) {
  switch (mode) {
    case VidOutMode::COLOR:
    case VidOutMode::COLOR_BG: tmcurses_set_color_palette(TMNCursesColorPalette::RGB); break;
    case VidOutMode::GRAY:
    case VidOutMode::GRAY_BG: tmcurses_set_color_palette(TMNCursesColorPalette::GRAYSCALE); break;
    case VidOutMode::PLAIN: break;
  }
}

void set_global_vom(VidOutMode* current, VidOutMode next) {
  init_global_video_output_mode(next);
  *current = next;
}
