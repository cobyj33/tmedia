#include "mediafetcher.h"

#include "audio.h"
#include "sleep.h"
#include "wtime.h"
#include "wmath.h"
#include "decode.h"

#include <mutex>
#include <chrono>
#include <system_error>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

void MediaFetcher::audio_dispatch_thread_func() {
  if (!this->has_media_stream(AVMEDIA_TYPE_AUDIO)) return;
  std::thread audio_fetching_thread;

  try {
    std::thread initialized_audio_fetching_thread(&MediaFetcher::audio_fetching_thread_func, this);
    audio_fetching_thread.swap(initialized_audio_fetching_thread);
  } catch (const std::system_error& err) {
    std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
    this->dispatch_exit(err.what());
  }

  if (audio_fetching_thread.joinable()) audio_fetching_thread.join();
}

void MediaFetcher::audio_fetching_thread_func() {
  static constexpr int AUDIO_THREAD_PAUSED_SLEEP_MS = 25;
  static constexpr int AUDIO_BUFFER_TRY_WRITE_WAIT_MS = 25;

  try { // super try block :)
    AudioResampler audio_resampler(
    this->media_decoder->get_ch_layout(), AV_SAMPLE_FMT_FLT, this->media_decoder->get_sample_rate(),
    this->media_decoder->get_ch_layout(), this->media_decoder->get_sample_fmt(), this->media_decoder->get_sample_rate());
    sleep_for_sec(this->media_decoder->get_start_time(AVMEDIA_TYPE_AUDIO));

    while (!this->should_exit()) {
      {
        std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
        while (!this->is_playing() && !this->should_exit()) {
          this->resume_cond.wait_for(resume_notify_lock, std::chrono::milliseconds(AUDIO_THREAD_PAUSED_SLEEP_MS));
        }
      }

      std::vector<AVFrame*> next_raw_audio_frames;

      {
        std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
        next_raw_audio_frames = this->media_decoder->next_frames(AVMEDIA_TYPE_AUDIO);
      }

      if (next_raw_audio_frames.size() != 0) {
        std::vector<AVFrame*> audio_frames = audio_resampler.resample_audio_frames(next_raw_audio_frames);
        
        for (int i = 0; i < (int)audio_frames.size(); i++) {
          while (!this->audio_buffer->try_write_into(audio_frames[i]->nb_samples, (float*)(audio_frames[i]->data[0]), AUDIO_BUFFER_TRY_WRITE_WAIT_MS)) {
            if (this->should_exit()) break;
          }
        }

        clear_av_frame_list(audio_frames);
      }
      clear_av_frame_list(next_raw_audio_frames);
    }
  } catch (std::exception const& err) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->dispatch_exit(err.what());
  }
}

