#include <tmedia/media/mediafetcher.h>

#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/ffmpeg/deleter.h>
#include <tmedia/ffmpeg/decode.h>
#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/audio/audio.h>
#include <tmedia/audio/audio_visualizer.h>
#include <tmedia/util/thread.h>
#include <tmedia/image/scale.h>
#include <tmedia/util/wtime.h>
#include <tmedia/ffmpeg/videoconverter.h>
#include <tmedia/util/defines.h>

#include <mutex>
#include <memory>
#include <stdexcept>
#include <chrono>

#include <fmt/format.h>

extern "C" {
#include <libavutil/version.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}

using namespace std::chrono_literals;

/**
 * The purpose of the video thread is to update the current MediaFetcher's
 * frame for rendering
 * 
 * This is done in different ways depending on if a video is available in the
 * current media type or not:
 * 
 * If the currently attached media is a video:
 *  The video thread will update the current frame to be the correct frame
 *  according to the MediaFetcher's current
 *  playback timestamp
 * 
 * If the currently attached media is an image:
 *  The video thread will read the cover art and exit
 * 
 * If the currently attached media is audio:
 *  If there is an attached cover art to the current audio file:
 *    The video thread will read the cover art and exit
 *  else:
 *    The video thread will update the current frame to be a snapshot of the
 *    wave of audio data currently being processed. 
*/


static constexpr std::chrono::milliseconds PAUSED_SLEEP_TIME_MS = 100ms;
static constexpr double DEFAULT_AVGFTS = 1.0 / 24.0;
static constexpr std::chrono::nanoseconds DEFAULT_AVGFTS_NS = secs_to_chns(DEFAULT_AVGFTS);

void MediaFetcher::video_fetching_thread_func() {
  // note that frame_audio_fetching_func can run even if there is no video data
  // available. Therefore, we can't just guard from AVMEDIA_TYPE_VIDEO here.
  name_current_thread("tmedia_vid_dec");

  try {
    if (this->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
      switch (this->media_type) {
        case MediaType::IMAGE: this->frame_image_fetching_func(); break;
        case MediaType::AUDIO: this->frame_audio_fetching_func(); break;
        case MediaType::VIDEO: this->frame_video_fetching_func(); break;
      }
    } else if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
      this->frame_audio_fetching_func();
    }
  } catch (std::exception const& err) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->dispatch_exit(err.what());
  }
}

void MediaFetcher::frame_video_fetching_func() {
  std::unique_ptr<AVFormatContext, AVFormatContextDeleter> fctx = open_fctx(this->path);
  OpenCCTXRes cctxavstr = open_cctx(fctx.get(), AVMEDIA_TYPE_VIDEO);
  AVCodecContext* cctx = cctxavstr.cctx.get();
  AVStream* avstr = cctxavstr.avstr;

  const Dim2 def_outdim = bound_dims(cctx->width * PAR_HEIGHT,
  cctx->height * PAR_WIDTH,
  MAX_FRAME_WIDTH,
  MAX_FRAME_HEIGHT);

  const double avg_fts =  1 / av_q2d(avstr->avg_frame_rate);
  VideoConverter vconv(def_outdim.width, def_outdim.height, AV_PIX_FMT_RGB24,
  cctx->width, cctx->height, cctx->pix_fmt);
  std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>> dec_frames;
  // single allocation of AVFrame buffer to use for the lifetime of the
  // video thread as output from convert_video_frame
  std::unique_ptr<AVFrame, AVFrameDeleter> converted_frame(av_frame_allocx());
  std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_allocx());
  vconv.configure_frame(converted_frame.get());

  while (!this->should_exit()) {
    if (!this->is_playing()) {
      std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
      while (!this->is_playing() && !this->should_exit()) {
        this->resume_cond.wait_for(resume_notify_lock, PAUSED_SLEEP_TIME_MS);
      }
    }

    {
      std::lock_guard<std::mutex> alter_mutex_lock(this->alter_mutex);
      if (this->req_dims) {
        Dim2 req_dims_bounded = bound_dims(
        cctx->width * PAR_HEIGHT,
        cctx->height * PAR_WIDTH,
        this->req_dims->width, this->req_dims->height);

        Dim2 out_dim = bound_dims(
        req_dims_bounded.width,
        req_dims_bounded.height,
        MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
        
        if (vconv.reset_dst_size(out_dim.width, out_dim.height)) {
          vconv.configure_frame(converted_frame.get());
        }
      }
    }
    
    double wait_duration = avg_fts;
    double current_time = 0.0;
    int msg_video_jump_curr_time_cache = 0;
    bool saved_frame_is_empty = false; // set in critical section

    {
      dec_frames.clear();
      decode_next_stream_frames(fctx.get(), cctx, avstr->index, packet.get(), dec_frames);
      std::scoped_lock<std::mutex> lock(this->alter_mutex);
      current_time = this->get_time(sys_clk_sec());
      msg_video_jump_curr_time_cache = this->msg_video_jump_curr_time;
      saved_frame_is_empty = this->frame.m_width == 0 || this->frame.m_height == 0;
    }

    if (msg_video_jump_curr_time_cache > 0) {
      int ret = av_jump_time(fctx.get(), cctx, avstr, packet.get(), current_time);
      if (unlikely(ret != 0)) {
        throw ffmpeg_error(fmt::format("[{}] Failed to jump to time {}", FUNCDINFO, current_time), ret);
      }

      dec_frames.clear();
      decode_next_stream_frames(fctx.get(), cctx, avstr->index, packet.get(), dec_frames);
      std::scoped_lock<std::mutex> lock(this->alter_mutex);
      this->msg_video_jump_curr_time--;
    }

    if (dec_frames.size() > 0) {
      const double frame_pts_time_sec = (double)dec_frames[0]->pts * av_q2d(avstr->time_base);
      const double extra_delay = (double)(dec_frames[0]->repeat_pict) / (2 * avg_fts);
      wait_duration = frame_pts_time_sec - current_time + extra_delay;

      if (wait_duration > 0.0 || saved_frame_is_empty) {
        vconv.convert_video_frame(converted_frame.get(), dec_frames.back().get());
        std::lock_guard<std::mutex> lock(this->alter_mutex);
        pixdata_from_avframe(this->frame, converted_frame.get());
        this->frame_changed = true;
      }

      dec_frames.clear();

      std::unique_lock<std::mutex> exit_lock(this->ex_noti_mtx);
      if (wait_duration > 0.0 && !this->should_exit()) {
        this->exit_cond.wait_for(exit_lock, secs_to_chns(wait_duration)); 
      }
    } else { // no frame was found.
      std::unique_lock<std::mutex> exit_lock(this->ex_noti_mtx);
      if (!this->should_exit()) {
        this->exit_cond.wait_for(exit_lock, DEFAULT_AVGFTS_NS); 
      }
    }

  }

}

void MediaFetcher::frame_image_fetching_func() {
  std::unique_ptr<AVFormatContext, AVFormatContextDeleter> fctx = open_fctx(this->path);
  OpenCCTXRes cctxavstr = open_cctx(fctx.get(), AVMEDIA_TYPE_VIDEO);
  std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_allocx());
  AVCodecContext* cctx = cctxavstr.cctx.get();
  AVStream* avstr = cctxavstr.avstr;

  Dim2 outdim = bound_dims(cctx->width * PAR_HEIGHT,
  cctx->height * PAR_WIDTH,
  MAX_FRAME_WIDTH,
  MAX_FRAME_HEIGHT);

  VideoConverter vconv(outdim.width,
  outdim.height,
  AV_PIX_FMT_RGB24,
  cctx->width,
  cctx->height,
  cctx->pix_fmt);

  std::unique_ptr<AVFrame, AVFrameDeleter> converted_frame(av_frame_allocx());
  vconv.configure_frame(converted_frame.get());
  std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>> dec_frames;
  decode_next_stream_frames(fctx.get(), cctx, avstr->index, packet.get(), dec_frames);
  if (dec_frames.size() > 0) {
    vconv.convert_video_frame(converted_frame.get(), dec_frames.back().get());
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    pixdata_from_avframe(this->frame, converted_frame.get());
    this->frame_changed = true;
  }
}

void MediaFetcher::frame_audio_fetching_func() {
  if (this->has_media_stream(AVMEDIA_TYPE_VIDEO)) { // assume attached pic
    try {
      this->frame_image_fetching_func();
      return;
    } catch (const std::runtime_error& e) {
      // no-op, Image decoding error, continue on with visualization
    }
  }

  static constexpr std::chrono::milliseconds AUDIO_PEEK_TRY_WAIT_MS = 100ms;
  static constexpr int AUDIO_PEEK_MAX_SAMPLE_SIZE = 2048;
  float audbuf[AUDIO_PEEK_MAX_SAMPLE_SIZE];

  const int nb_ch = this->audio_buffer->get_nb_channels();
  const int aubduf_nb_frames = AUDIO_PEEK_MAX_SAMPLE_SIZE / nb_ch;
  Dim2 visdim{MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT};

  {
    std::scoped_lock<std::mutex> alter_lock(this->alter_mutex);
    if (this->req_dims) {
      visdim = bound_dims(
        this->req_dims->width,
        this->req_dims->height,
        MAX_FRAME_WIDTH,
        MAX_FRAME_HEIGHT);
    }
  }

  /**
   * I had originally thought that maybe the audio visualizer could have it's 
   * own MediaDecoder decoding audio to visualize, but peeking into the
   * MediaFetcher's buffer doesn't seem to cause any problems at all, and
   * greatly simplifies syncing the visualization with actual audio output
   * from the MediaFetcher.
  */

  PixelData local_frame;
  pixdata_initgray(local_frame, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, 0);
  
  while (!this->should_exit()) {
    if (!this->is_playing()) {
      std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
      while (!this->is_playing() && !this->should_exit()) {
        this->resume_cond.wait_for(resume_notify_lock, PAUSED_SLEEP_TIME_MS);
      }
    }


    if (this->audio_buffer->try_peek_into(aubduf_nb_frames, audbuf, AUDIO_PEEK_TRY_WAIT_MS)) {
      visualize(local_frame, audbuf, aubduf_nb_frames, nb_ch, visdim.width, visdim.height);
      std::scoped_lock<std::mutex> alter_lock(this->alter_mutex);
      pixdata_copy(this->frame, local_frame);
      this->frame_changed = true;
      if (this->req_dims) {
        visdim = bound_dims(
        this->req_dims->width,
        this->req_dims->height,
        MAX_FRAME_WIDTH,
        MAX_FRAME_HEIGHT);
      }
    }


    std::unique_lock<std::mutex> exit_lock(this->ex_noti_mtx);
    if (!this->should_exit()) {
      this->exit_cond.wait_for(exit_lock, DEFAULT_AVGFTS_NS); 
    }
  }
}
