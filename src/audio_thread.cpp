
#include "boiler.h"
#include "decode.h"
#include "media.h"
#include "wtime.h"
#include "audio.h"
#include "wmath.h"
#include "except.h"
#include "pixeldata.h"
#include "sleep.h"
#include "audioresampler.h"
#include "avguard.h"

#include <cstdlib>
#include <memory>
#include <mutex>


extern "C" {
#include <miniaudio.h>

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

/**
 * @brief The callback called by miniaudio once the connected audio device requests audio data
 * 
 * @param pDevice The miniaudio audio device representation
 * @param pOutput The memory buffer to write frameCount samples into
 * @param pInput Irrelevant for us :)
 * @param sampleCount The number of samples requested by miniaudio
 */
void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 sampleCount)
{
  // CallbackData* data = (CallbackData*)(pDevice->pUserData);
  MediaPlayer* player = (MediaPlayer*)(pDevice->pUserData);
  std::lock_guard<std::mutex> mutex_lock_guard(player->buffer_read_mutex);

  while (!player->audio_buffer->can_read(sampleCount)) {
    int written_samples = player->load_next_audio();
    if (written_samples == 0) {
      break;
    }
  }

  if (player->audio_buffer->can_read(sampleCount)) {
    player->audio_buffer->read_into(sampleCount, (float*)pOutput);
  } else {
    float* pFloatOutput = (float*)pOutput;
    for (ma_uint32 i = 0; i < sampleCount; i++) {
      pFloatOutput[i] = 0.0;
    }
  }

  (void)pInput;
}

void MediaPlayer::audio_playback_thread() {
  ma_result miniaudio_log;
  ma_device audio_device;
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  double START_TIME = 0.0;

  {
    std::lock_guard<std::mutex> mutex_lock(this->alter_mutex);
    if (!this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      throw std::runtime_error("[MediaPlayer::audio_playback_thread] Cannot play audio playback, Audio stream could not be found");
    }

    config.playback.format  = ma_format_f32;
    config.playback.channels = this->media_decoder->get_nb_channels();
    config.sampleRate = this->media_decoder->get_sample_rate();       
    config.dataCallback = audioDataCallback;   
    config.pUserData = this;
    START_TIME = this->media_decoder->get_start_time(AVMEDIA_TYPE_AUDIO);
  }

  miniaudio_log = ma_device_init(NULL, &config, &audio_device);

  if (miniaudio_log != MA_SUCCESS) {
    throw std::runtime_error("[MediaPlayer::audio_playback_thread] FAILED TO INITIALIZE AUDIO DEVICE: " + std::to_string(miniaudio_log));
  }
  miniaudio_log = ma_device_start(&audio_device);
  if (miniaudio_log != MA_SUCCESS) {
    ma_device_uninit(&audio_device);
    throw std::runtime_error("[MediaPlayer::audio_playback_thread] Failed to initially start playback: Miniaudio Error " + std::to_string(miniaudio_log));
  }
  miniaudio_log = ma_device_stop(&audio_device);
  if (miniaudio_log != MA_SUCCESS) {
    ma_device_uninit(&audio_device);
    throw std::runtime_error("[MediaPlayer::audio_playback_thread] Failed to initially stop playback: Miniaudio Error " + std::to_string(miniaudio_log));
  }

  sleep_for_sec(START_TIME);
  
  while (true) {
    {
      std::lock_guard<std::mutex> mutex_lock(this->alter_mutex);
      if (!this->in_use)
        break;
      
      ma_device_state audio_device_state = ma_device_get_state(&audio_device);

      if ((this->clock.is_playing() == false || this->muted) && audio_device_state == ma_device_state_started) { // audio stop and start handling
        miniaudio_log = ma_device_stop(&audio_device);
        if (miniaudio_log != MA_SUCCESS) {
          ma_device_uninit(&audio_device);
          throw std::runtime_error("[MediaPlayer::audio_playback_thread] Failed to stop playback: Miniaudio Error " + std::to_string(miniaudio_log));
        }
      } else if ((this->clock.is_playing() && !this->muted) && audio_device_state == ma_device_state_stopped) {
        miniaudio_log = ma_device_start(&audio_device);
        if (miniaudio_log != MA_SUCCESS) {
          ma_device_uninit(&audio_device);
          throw std::runtime_error("[MediaPlayer::audio_playback_thread] Failed to start playback: Miniaudio Error " + std::to_string(miniaudio_log));
        }
      }

      audio_device_state = ma_device_get_state(&audio_device);
      if (audio_device_state == ma_device_state_stopped || audio_device_state == ma_device_state_stopping) {
        sleep_quick();
        continue;
      }

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

      audio_device.sampleRate = this->media_decoder->get_sample_rate() * this->clock.get_speed();
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

  ma_device_uninit(&audio_device);
}

