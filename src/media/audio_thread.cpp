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

using namespace std::chrono_literals;

void MediaFetcher::audio_dispatch_thread_func() {
  if (!this->has_media_stream(AVMEDIA_TYPE_AUDIO)) return;
  
  static constexpr std::chrono::milliseconds AUDIO_THREAD_PAUSED_SLEEP_MS = 25ms;
  static constexpr std::chrono::milliseconds AUDIO_BUFFER_TRY_WRITE_WAIT_MS = 25ms;

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

    /*
      There was an issue in tmedia where at the end of an audio stream, CPU
      Usage would spike up to 100% since the MediaFetcher would constantly
      be trying to decode audio, but there was no more audio to decode, so
      the decoded AVFrame vector kept having a size of zero. These variables
      exist to throttle the amount of times no audio can
      be decoded before the audio thread is forced to wait for a small timeout.

      runs_w_fail increases everytime decoding AVFrames from the MediaDecoder
      results in no decoded audio. and if runs_w_fail exceeds MAX_RUNS_W_FAIL,
      the current thread is forced to sleep for MAX_RUNS_WAIT_TIME. Everytime
      the current thread is forced to sleep due to
      runs_w_fail > MAX_RUNS_W_FAIL, runs_w_fail is reset to 0. Additionally,
      anytime any AVFrames are successfully decoded from the MediaDecoder,
      runs_w_fail is reset to 0.

      MAX_RUNS_WAIT_TIME shouldn't be too long, as the MediaFetcher could jump
      times at any time, and the audio thread shouldn't be sleep for long
      whenever that takes place.
    */

    static constexpr std::chrono::milliseconds MAX_RUNS_WAIT_TIME(25);
    static constexpr unsigned int MAX_RUNS_W_FAIL = 5;
    unsigned int runs_w_fail = 0;
    std::vector<AVFrame*> next_raw_audio_frames;

    while (!this->should_exit()) {
      if (!this->is_playing()) {
        std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
        while (!this->is_playing() && !this->should_exit()) {
          this->resume_cond.wait_for(resume_notify_lock, AUDIO_THREAD_PAUSED_SLEEP_MS);
        }
      }

      int msg_audio_jump_curr_time_cache = 0;
      double current_time = 0;
      // next_raw_audio_frames is cleared already from botton of loop

      {
        clear_avframe_list(next_raw_audio_frames);
        adec.next_frames(AVMEDIA_TYPE_AUDIO, next_raw_audio_frames);
        std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
        current_time = this->clock.get_time(sys_clk_sec());
        msg_audio_jump_curr_time_cache = this->msg_audio_jump_curr_time;
      }

      if (msg_audio_jump_curr_time_cache > 0) {
        adec.jump_to_time(current_time);
        clear_avframe_list(next_raw_audio_frames);
        adec.next_frames(AVMEDIA_TYPE_AUDIO, next_raw_audio_frames);
        // clear audio buffer **after** expensive time jumping functions and
        // decoding functions
        this->audio_buffer->clear(current_time);
        std::scoped_lock<std::mutex> lock(this->alter_mutex);
        this->msg_audio_jump_curr_time--;
      }

      runs_w_fail += static_cast<unsigned int>(next_raw_audio_frames.size() == 0);
      for (std::size_t i = 0; i < next_raw_audio_frames.size(); i++) {
        runs_w_fail = 0;
        AVFrame* frame = audio_resampler.resample_audio_frame(next_raw_audio_frames[i]);
        while (!this->audio_buffer->try_write_into(frame->nb_samples, (float*)(frame->data[0]), AUDIO_BUFFER_TRY_WRITE_WAIT_MS)) {
          if (this->should_exit()) break;
        }
        av_frame_free(&frame);
      }

      clear_avframe_list(next_raw_audio_frames);
      if (runs_w_fail >= MAX_RUNS_W_FAIL) {
        runs_w_fail = 0;
        std::unique_lock<std::mutex> exit_lock(this->ex_noti_mtx);
        if (!this->should_exit()) {
          this->exit_cond.wait_for(exit_lock, MAX_RUNS_WAIT_TIME);
        }
      }
    }
  } catch (std::exception const& err) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->dispatch_exit(err.what());
  }

}

