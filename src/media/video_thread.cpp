#include <tmedia/media/mediafetcher.h>

#include <tmedia/ffmpeg/decode.h>
#include <tmedia/audio/audio.h>
#include <tmedia/audio/audio_visualizer.h>
#include <tmedia/util/sleep.h>
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

// Pixel Aspect Ratio - account for tall rectangular shape of terminal
//characters
static constexpr int PAR_WIDTH = 2;
static constexpr int PAR_HEIGHT = 5;

static constexpr int MAX_FRAME_ASPECT_RATIO_WIDTH = 16 * PAR_HEIGHT;
static constexpr int MAX_FRAME_ASPECT_RATIO_HEIGHT = 9 * PAR_WIDTH;
static constexpr double MAX_FRAME_ASPECT_RATIO = static_cast<double>(MAX_FRAME_ASPECT_RATIO_WIDTH) / static_cast<double>(MAX_FRAME_ASPECT_RATIO_HEIGHT);

// I found that past a width of 640 characters,
// the terminal starts to stutter terribly on most terminal emulators, so we
// just bound the image to this amount

static constexpr int MAX_FRAME_WIDTH = 640;
static constexpr int MAX_FRAME_HEIGHT = static_cast<int>(static_cast<double>(MAX_FRAME_WIDTH) / MAX_FRAME_ASPECT_RATIO);
static constexpr std::chrono::milliseconds PAUSED_SLEEP_TIME_MS = 100ms;
static constexpr double DEFAULT_AVGFTS = 1.0 / 24.0;
static constexpr std::chrono::nanoseconds DEFAULT_AVGFTS_NS = secs_to_chns(DEFAULT_AVGFTS);

void MediaFetcher::video_fetching_thread_func() {
  // note that frame_audio_fetching_func can run even if there is no video data
  // available. Therefore, we can't just guard from AVMEDIA_TYPE_VIDEO here.

  try {
    switch (this->media_type) {
      case MediaType::IMAGE: this->frame_image_fetching_func(); break;
      case MediaType::VIDEO: this->frame_video_fetching_func(); break;
      case MediaType::AUDIO: this->frame_audio_fetching_func(); break;
      default: return;
    }
  } catch (std::exception const& err) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->dispatch_exit(err.what());
  }
}

void MediaFetcher::frame_video_fetching_func() {
  MediaDecoder vdec(this->mdec->path, { AVMEDIA_TYPE_VIDEO });
  if (!vdec.has_stream_decoder(AVMEDIA_TYPE_VIDEO)) return; // copy failed?

  const Dim2 def_outdim = bound_dims(vdec.get_width() * PAR_HEIGHT,
  vdec.get_height() * PAR_WIDTH,
  MAX_FRAME_WIDTH,
  MAX_FRAME_HEIGHT);

  const double avg_fts = vdec.get_avgfts(AVMEDIA_TYPE_VIDEO);
  VideoConverter vconv(def_outdim.width, def_outdim.height, AV_PIX_FMT_RGB24,
  vdec.get_width(), vdec.get_height(), vdec.get_pix_fmt());

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
        vdec.get_width() * PAR_HEIGHT,
        vdec.get_height() * PAR_WIDTH,
        this->req_dims->width, this->req_dims->height);

        Dim2 out_dim = bound_dims(
        req_dims_bounded.width,
        req_dims_bounded.height,
        MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
        
        vconv.reset_dst_size(out_dim.width, out_dim.height);
      }
    }
    
    double wait_duration = avg_fts;
    std::vector<AVFrame*> dec_frames;
    double current_time = 0.0;
    int msg_video_jump_curr_time_cache = 0;
    {
      dec_frames = vdec.next_frames(AVMEDIA_TYPE_VIDEO);
      std::scoped_lock<std::mutex> lock(this->alter_mutex);
      current_time = this->get_time(sys_clk_sec());
      msg_video_jump_curr_time_cache = this->msg_video_jump_curr_time;
    }

    if (msg_video_jump_curr_time_cache > 0) {
      vdec.jump_to_time(current_time);
      dec_frames = vdec.next_frames(AVMEDIA_TYPE_VIDEO);
      std::scoped_lock<std::mutex> lock(this->alter_mutex);
      this->msg_video_jump_curr_time--;
    }

    if (dec_frames.size() > 0) {
      const double frame_pts_time_sec = (double)dec_frames[0]->pts * vdec.get_time_base(AVMEDIA_TYPE_VIDEO);
      const double extra_delay = (double)(dec_frames[0]->repeat_pict) / (2 * avg_fts);
      wait_duration = frame_pts_time_sec - current_time + extra_delay;

      if (wait_duration > 0.0 || this->frame.get_width() * this->frame.get_height() == 0) { // or the current frame has no valid dimensions
        AVFrame* frame_image = vconv.convert_video_frame(dec_frames[0]);
        PixelData pix_data = PixelData(frame_image);
        av_frame_free(&frame_image);
        
        std::lock_guard<std::mutex> lock(this->alter_mutex);
        this->frame = pix_data;
      }
      clear_avframe_list(dec_frames);
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
  MediaDecoder vdec(this->mdec->path, { AVMEDIA_TYPE_VIDEO });

  Dim2 outdim = bound_dims(vdec.get_width() * PAR_HEIGHT,
  vdec.get_height() * PAR_WIDTH,
  MAX_FRAME_WIDTH,
  MAX_FRAME_HEIGHT);

  VideoConverter vconv(outdim.width,
  outdim.height,
  AV_PIX_FMT_RGB24,
  vdec.get_width(),
  vdec.get_height(),
  vdec.get_pix_fmt());

  std::vector<AVFrame*> dec_frames = vdec.next_frames(AVMEDIA_TYPE_VIDEO);
  if (dec_frames.size() > 0) {
    AVFrame* frame_image = vconv.convert_video_frame(dec_frames[0]);
    PixelData frame_pixel_data = PixelData(frame_image);
    {
      std::lock_guard<std::mutex> lock(this->alter_mutex);
      this->frame = frame_pixel_data;
    }
    av_frame_free(&frame_image);
  }
  clear_avframe_list(dec_frames);
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

  static constexpr int AUDIO_PEEK_TRY_WAIT_MS = 100;
  static constexpr int AUDIO_PEEK_MAX_SAMPLE_SIZE = 2048;
  float audbuf[AUDIO_PEEK_MAX_SAMPLE_SIZE];

  const int nb_ch = this->audio_buffer->get_nb_channels();
  const int aubduf_sz = AUDIO_PEEK_MAX_SAMPLE_SIZE / nb_ch;
  Dim2 visdim(MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
  if (this->req_dims) {
    visdim = bound_dims(
    this->req_dims->width,
    this->req_dims->height,
    MAX_FRAME_WIDTH,
    MAX_FRAME_HEIGHT);
  }

  /**
   * I had originally thought that maybe the audio visualizer could have it's
   * own MediaDecoder decoding audio to visualize, but peeking into the
   * MediaFetcher's buffer doesn't seem to cause any problems at all, and
   * greatly simplifies syncing the visualization with actual audio output
   * from the MediaFetcher.
  */
  
  while (!this->should_exit()) {
    if (!this->is_playing()) {
      std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
      while (!this->is_playing() && !this->should_exit()) {
        this->resume_cond.wait_for(resume_notify_lock, PAUSED_SLEEP_TIME_MS);
      }
    }


    if (this->audio_buffer->try_peek_into(aubduf_sz, audbuf, AUDIO_PEEK_TRY_WAIT_MS)) {
      PixelData frame = visualize(audbuf, aubduf_sz, nb_ch, visdim.width, visdim.height);
      std::scoped_lock<std::mutex> alter_lock(this->alter_mutex);
      this->frame = frame;
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
