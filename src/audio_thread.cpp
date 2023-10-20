#include "mediafetcher.h"

#include "audio.h"
#include "sleep.h"
#include "wtime.h"
#include "wmath.h"

#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}


const int RECOMMENDED_AUDIO_BUFFER_SIZE = 44100 * 15; // 15 seconds of audio data at 44100 Hz sample rate
const double MAX_AUDIO_BUFFER_TIME_BEFORE_SECONDS = 120.0;
const double RESET_AUDIO_BUFFER_TIME_BEFORE_SECONDS = 15.0;
const double MAX_AUDIO_DESYNC_TIME_SECONDS = 0.25;
const double MAX_AUDIO_CATCHUP_DECODE_TIME_SECONDS = 2.5;
const int AUDIO_FRAME_INCREMENTAL_LOAD_AMOUNT = 5;
const int AUDIO_THREAD_ITERATION_SLEEP_MS = 5;
const int MINIMUM_AUDIO_BUFFER_DEVICE_START_SIZE = 1024;

void MediaFetcher::audio_fetching_thread() {
  try { // super try block :)
    sleep_for_sec(this->media_decoder->get_start_time(AVMEDIA_TYPE_AUDIO));
    
    while (this->in_use) {
      if (!this->clock.is_playing()) {
        sleep_quick();
        continue;
      }

      {
        std::lock_guard<std::mutex> mutex_lock(this->alter_mutex);
        const double current_system_time = system_clock_sec();
        const double target_resync_time = this->get_time(current_system_time);
        if (target_resync_time > this->get_duration()) {
          this->in_use = false;
          break;
        }

        std::lock_guard<std::mutex> lock(this->buffer_read_mutex);

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
        } // end of desync handling

        if (this->audio_buffer->get_elapsed_time() > MAX_AUDIO_BUFFER_TIME_BEFORE_SECONDS) {
          this->audio_buffer->leave_behind(RESET_AUDIO_BUFFER_TIME_BEFORE_SECONDS);
        }

      }

      {
        std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
        if (!this->audio_buffer->can_read(RECOMMENDED_AUDIO_BUFFER_SIZE)) {
          for (int i = 0; i < AUDIO_FRAME_INCREMENTAL_LOAD_AMOUNT; i++) {
            std::lock_guard<std::mutex> lock(this->buffer_read_mutex);
            this->load_next_audio();
          }
        }
      }

    }

  } catch (std::exception const& e) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->error = std::string(e.what());
    this->in_use = false;
  }

}

