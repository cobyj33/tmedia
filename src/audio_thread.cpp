#include "mediafetcher.h"

#include "audio.h"
#include "sleep.h"
#include "wtime.h"
#include "wmath.h"
#include "decode.h"

#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

void MediaFetcher::audio_dispatch_thread_func() {
  if (!this->has_media_stream(AVMEDIA_TYPE_AUDIO)) return;
  std::thread audio_sync_thread;
  std::thread buffer_size_management_thread;
  std::thread audio_fetching_thread;

  try {
    std::thread initialized_audio_sync_thread(&MediaFetcher::audio_sync_thread_func, this);
    audio_sync_thread.swap(initialized_audio_sync_thread);

    std::thread initialized_buffer_size_management_thread(&MediaFetcher::buffer_size_management_thread_func, this);
    buffer_size_management_thread.swap(initialized_buffer_size_management_thread);

    std::thread initialized_audio_fetching_thread(&MediaFetcher::audio_fetching_thread_func, this);
    audio_fetching_thread.swap(initialized_audio_fetching_thread);
  } catch (const std::system_error& err) {
    std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
    this->error = err.what();
    this->in_use = false;
  }

  if (audio_sync_thread.joinable()) audio_sync_thread.join();
  if (buffer_size_management_thread.joinable()) buffer_size_management_thread.join();
  if (audio_fetching_thread.joinable()) audio_fetching_thread.join();
}

void MediaFetcher::audio_sync_thread_func() {
  const double MAX_AUDIO_DESYNC_TIME_SECONDS = 0.25;
  const double MAX_AUDIO_CATCHUP_DECODE_TIME_SECONDS = 2.5;
  const int AUDIO_SYNC_THREAD_ITERATION_SLEEP_MS = 200;

  if (!this->has_media_stream(AVMEDIA_TYPE_VIDEO)) return;

  try {
    while (this->in_use) {
      {
        std::scoped_lock<std::mutex, std::mutex> total_lock{this->alter_mutex, this->audio_buffer_mutex};
        const double current_system_time = system_clock_sec();
        const double target_resync_time = this->get_time(current_system_time);
        if (target_resync_time > this->get_duration()) {
          this->in_use = false;
          break;
        }


        if (this->get_desync_time(current_system_time) > MAX_AUDIO_DESYNC_TIME_SECONDS) { // desync handling
          if (this->audio_buffer->is_time_in_bounds(target_resync_time)) {
            this->audio_buffer->set_time_in_bounds(target_resync_time);
          } else if (target_resync_time > this->audio_buffer->get_time() && target_resync_time <= this->audio_buffer->get_time() + MAX_AUDIO_CATCHUP_DECODE_TIME_SECONDS) {
            int writtenSamples = 0;
            do {
              writtenSamples = this->load_next_audio();
            } while (writtenSamples != 0 && !this->audio_buffer->is_time_in_bounds(target_resync_time));

            if (this->audio_buffer->is_time_in_bounds(target_resync_time)) {
              this->audio_buffer->set_time_in_bounds(target_resync_time);
            } else {
              this->jump_to_time(target_resync_time, current_system_time);
            }
          } else {
            this->jump_to_time(target_resync_time, current_system_time);
          }
        }
      }

      sleep_for_ms(AUDIO_SYNC_THREAD_ITERATION_SLEEP_MS);
    }
  } catch (const std::exception& err) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->error = std::string(err.what());
    this->in_use = false;
  }

}

void MediaFetcher::buffer_size_management_thread_func() {
  const int AUDIO_BUFFER_SIZE_MANAGEMENT_ITERATION_SLEEP_MS = 500;
  const double MAX_AUDIO_BUFFER_TIME_BEFORE_SECONDS = 30.0;
  const double RESET_AUDIO_BUFFER_TIME_BEFORE_SECONDS = 0.0;

  try {
      while (this->in_use) {
        {
          std::lock_guard<std::mutex> audio_buffer_lock(this->audio_buffer_mutex);
          if (this->audio_buffer->get_elapsed_time() > MAX_AUDIO_BUFFER_TIME_BEFORE_SECONDS) {
            this->audio_buffer->leave_behind(RESET_AUDIO_BUFFER_TIME_BEFORE_SECONDS);
          }
        }

        sleep_for_ms(AUDIO_BUFFER_SIZE_MANAGEMENT_ITERATION_SLEEP_MS);
      }
  } catch (const std::exception& err) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->error = err.what();
    this->in_use = false;
  }
}

void MediaFetcher::audio_fetching_thread_func() {
  const int AUDIO_BUFFER_SHOULD_READ_SIZE = 44100; // 1 seconds of audio data at 44100 Hz sample rate
  const int AUDIO_FRAME_INCREMENTAL_LOAD_AMOUNT = 10;
  const int AUDIO_THREAD_ITERATION_SLEEP_MS = 17;
  const int AUDIO_THREAD_REST_SLEEP_MS = 170;

  try { // super try block :)
    AudioResampler audio_resampler(
    this->media_decoder->get_ch_layout(), AV_SAMPLE_FMT_FLT, this->media_decoder->get_sample_rate(),
    this->media_decoder->get_ch_layout(), this->media_decoder->get_sample_fmt(), this->media_decoder->get_sample_rate());
    
    sleep_for_sec(this->media_decoder->get_start_time(AVMEDIA_TYPE_AUDIO));

    while (this->in_use) {
      if (!this->clock.is_playing()) {
        sleep_for_ms(AUDIO_THREAD_ITERATION_SLEEP_MS);
        continue;
      }

      bool can_rest = true;
      bool should_load = false;
      {
        std::lock_guard<std::mutex> audio_buffer_lock(this->audio_buffer_mutex);
        should_load = !this->audio_buffer->can_read(AUDIO_BUFFER_SHOULD_READ_SIZE);
      }

      if (should_load) {
        for (int i = 0; i < AUDIO_FRAME_INCREMENTAL_LOAD_AMOUNT; i++) {
          std::vector<AVFrame*> next_raw_audio_frames;

          {
            std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
            next_raw_audio_frames = this->media_decoder->next_frames(AVMEDIA_TYPE_AUDIO);
          }

          if (next_raw_audio_frames.size() == 0) {
            can_rest = true;
            break;
          }
          
          std::vector<AVFrame*> audio_frames = audio_resampler.resample_audio_frames(next_raw_audio_frames);

          for (int i = 0; i < (int)audio_frames.size(); i++) {
            std::lock_guard<std::mutex> audio_buffer_lock(this->audio_buffer_mutex);
            this->audio_buffer->write((float*)(audio_frames[i]->data[0]), audio_frames[i]->nb_samples);
            can_rest = this->audio_buffer->can_read(AUDIO_BUFFER_SHOULD_READ_SIZE);
          }

          clear_av_frame_list(next_raw_audio_frames);
          clear_av_frame_list(audio_frames);
        }
      }

      if (can_rest)
        sleep_for_ms(AUDIO_THREAD_REST_SLEEP_MS);
    }

  } catch (std::exception const& e) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->error = std::string(e.what());
    this->in_use = false;
  }

}

