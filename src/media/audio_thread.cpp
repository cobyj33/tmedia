#include <tmedia/media/mediafetcher.h>

#include <tmedia/audio/audio.h>
#include <tmedia/util/sleep.h>
#include <tmedia/util/wtime.h>
#include <tmedia/util/wmath.h>
#include <tmedia/ffmpeg/decode.h>
#include <tmedia/ffmpeg/audioresampler.h>

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
  
  static constexpr int AUDIO_THREAD_PAUSED_SLEEP_MS = 25;
  static constexpr int AUDIO_BUFFER_TRY_WRITE_WAIT_MS = 25;

  try { // super try block :)
    std::array<bool, AVMEDIA_TYPE_NB> requested_streams;
    for (std::size_t i = 0; i < requested_streams.size(); i++) requested_streams[i] = false;
    requested_streams[AVMEDIA_TYPE_AUDIO] = true;
    MediaDecoder adec(this->mdec->path, requested_streams);
    if (!adec.has_stream_decoder(AVMEDIA_TYPE_AUDIO)) return; // copy failed?
    AudioResampler audio_resampler(
    adec.get_ch_layout(), AV_SAMPLE_FMT_FLT, adec.get_sample_rate(),
    adec.get_ch_layout(), adec.get_sample_fmt(), adec.get_sample_rate());
    sleep_for_sec(adec.get_start_time(AVMEDIA_TYPE_AUDIO));

    while (!this->should_exit()) {
      if (!this->is_playing()) {
        std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
        while (!this->is_playing() && !this->should_exit()) {
          this->resume_cond.wait_for(resume_notify_lock, std::chrono::milliseconds(AUDIO_THREAD_PAUSED_SLEEP_MS));
        }
      }

      std::vector<AVFrame*> next_raw_audio_frames;
      int msg_audio_jump_curr_time_cache = 0;
      double current_time = 0;

      {
        next_raw_audio_frames = adec.next_frames(AVMEDIA_TYPE_AUDIO);
        std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
        current_time = this->clock.get_time(sys_clk_sec());
        msg_audio_jump_curr_time_cache = this->msg_audio_jump_curr_time;
      }

      if (msg_audio_jump_curr_time_cache > 0) {
        this->audio_buffer->clear(current_time);
        adec.jump_to_time(current_time);
        next_raw_audio_frames = adec.next_frames(AVMEDIA_TYPE_AUDIO);
        
        std::scoped_lock<std::mutex> lock(this->alter_mutex);
        this->msg_audio_jump_curr_time--;
      }

      for (std::size_t i = 0; i < next_raw_audio_frames.size(); i++) {
        AVFrame* frame = audio_resampler.resample_audio_frame(next_raw_audio_frames[i]);
        while (!this->audio_buffer->try_write_into(frame->nb_samples, (float*)(frame->data[0]), AUDIO_BUFFER_TRY_WRITE_WAIT_MS)) {
          if (this->should_exit()) break;
        }
        av_frame_free(&frame);
      }
      clear_avframe_list(next_raw_audio_frames);
    }
  } catch (std::exception const& err) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->dispatch_exit(err.what());
  }
}

