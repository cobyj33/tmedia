#include <tmedia/media/mediafetcher.h>

#include <tmedia/audio/audio.h>
#include <tmedia/util/thread.h>
#include <tmedia/util/wtime.h>
#include <tmedia/util/wmath.h>
#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/ffmpeg/deleter.h>
#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/ffmpeg/decode.h>
#include <tmedia/ffmpeg/audioresampler.h>

#include <mutex>
#include <chrono>

#include <fmt/format.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

using namespace std::chrono_literals;

void MediaFetcher::audio_dispatch_thread_func() {
  if (!this->has_media_stream(AVMEDIA_TYPE_AUDIO)) return;
  name_current_thread("tmedia_aud_dec");
  
  static constexpr std::chrono::milliseconds AUDIO_THREAD_PAUSED_SLEEP_MS = 25ms;
  static constexpr std::chrono::milliseconds AUDIO_BUFFER_TRY_WRITE_WAIT_MS = 25ms;

  try { // super try block :)
    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> fctx = open_fctx(this->path);
    OpenCCTXRes cctxavstr = open_cctx(fctx.get(), AVMEDIA_TYPE_AUDIO);
    AVCodecContext* cctx = cctxavstr.cctx.get();
    AVStream* avstr = cctxavstr.avstr;

    AudioResampler audio_resampler(
    cctx_get_ch_layout(cctx), AV_SAMPLE_FMT_FLT, cctx->sample_rate,
    cctx_get_ch_layout(cctx), cctx->sample_fmt, cctx->sample_rate);

    sleep_for_sec(avstr_get_start_time(avstr));

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

    std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>> next_raw_audio_frames, frame_pool;
    std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_allocx());

    // single resampled frame buffer to use for the lifetime of the
    // audio thread
    std::unique_ptr<AVFrame, AVFrameDeleter> resampled_frame(av_frame_allocx());

    while (!this->should_exit()) {
      if (!this->is_playing()) {
        std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
        while (!this->is_playing() && !this->should_exit()) {
          this->resume_cond.wait_for(resume_notify_lock, AUDIO_THREAD_PAUSED_SLEEP_MS);
        }
      }

      int msg_audio_jump_curr_time_cache = 0;
      double current_time = 0;

      {
        decode_next_stream_frames(fctx.get(), cctx, avstr->index, packet.get(), next_raw_audio_frames, frame_pool);
        std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
        current_time = this->clock.get_time(sys_clk_sec());
        msg_audio_jump_curr_time_cache = this->msg_audio_jump_curr_time;
      }

      if (msg_audio_jump_curr_time_cache > 0) {
        int ret = av_jump_time(fctx.get(), cctx, avstr, packet.get(), current_time);
        if (unlikely(ret != 0)) {
          throw ffmpeg_error(fmt::format("[{}] Failed to jump to time {}", FUNCDINFO, current_time), ret);
        }

        decode_next_stream_frames(fctx.get(), cctx, avstr->index, packet.get(), next_raw_audio_frames, frame_pool);

        // clear audio buffer **after** expensive time jumping functions and
        // decoding functions
        this->audio_buffer->clear(current_time);
        std::scoped_lock<std::mutex> lock(this->alter_mutex);
        this->msg_audio_jump_curr_time--;
      }

      runs_w_fail += static_cast<unsigned int>(next_raw_audio_frames.size() == 0);
      for (std::size_t i = 0; i < next_raw_audio_frames.size(); i++) {
        runs_w_fail = 0;
        audio_resampler.resample_audio_frame(resampled_frame.get(), next_raw_audio_frames[i].get());
        while (!this->audio_buffer->try_write_into(resampled_frame->nb_samples, (float*)(resampled_frame->data[0]), AUDIO_BUFFER_TRY_WRITE_WAIT_MS)) {
          if (this->should_exit()) break;
        }
      }

      // reading possible extra buffered data
      audio_resampler.resample_audio_frame(resampled_frame.get(), nullptr);
      while (resampled_frame->nb_samples > 0) {
        while (!this->audio_buffer->try_write_into(resampled_frame->nb_samples, (float*)(resampled_frame->data[0]), AUDIO_BUFFER_TRY_WRITE_WAIT_MS)) {
          if (this->should_exit()) break;
        }
        audio_resampler.resample_audio_frame(resampled_frame.get(), nullptr);
      }

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

