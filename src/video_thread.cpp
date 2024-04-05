#include "mediafetcher.h"

#include "decode.h"
#include "audio.h"
#include "audio_visualizer.h"
#include "sleep.h"
#include "scale.h"
#include "wtime.h"
#include "videoconverter.h"
#include "funcmac.h"

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

/**
 * The purpose of the video thread is to update the current MediaFetcher's frame for rendering
 * 
 * This is done in different ways depending on if a video is available in the current media type or not
 * 
 * If the currently attached media is a video:
 *  The video thread will update the current frame to be the correct frame according to the MediaFetcher's current
 *  playback timestamp
 * 
 * If the currently attached media is an image:
 *  The video thread will read the cover art and exit
 * 
 * If the currently attached media is audio:
 *  If there is an attached cover art to the current audio file:
 *    The video thread will read the cover art and exit
 *  else:
 *    The video thread will update the current frame to be a snapshot of the wave of audio data currently being
 *    processed. 
*/

const int PAR_WIDTH = 2; // account for non-square shape of terminal characters
const int PAR_HEIGHT = 5;

const int MAX_FRAME_ASPECT_RATIO_WIDTH = 16 * PAR_HEIGHT;
const int MAX_FRAME_ASPECT_RATIO_HEIGHT = 9 * PAR_WIDTH;
const double MAX_FRAME_ASPECT_RATIO = (double)MAX_FRAME_ASPECT_RATIO_WIDTH / (double)MAX_FRAME_ASPECT_RATIO_HEIGHT;

// I found that past a width of 640 characters,
// the terminal starts to stutter terribly on most terminal emulators, so we just
// bound the image to this amount

const int MAX_FRAME_WIDTH = 640;
const int MAX_FRAME_HEIGHT = MAX_FRAME_WIDTH / MAX_FRAME_ASPECT_RATIO;
const int PAUSED_SLEEP_TIME_MS = 100;
const double DEFAULT_AVGFTS = 1.0 / 24.0;

void MediaFetcher::video_fetching_thread_func() {
  try {
    switch (this->media_type) {
      case MediaType::IMAGE: this->frame_image_fetching_func(); break;
      case MediaType::VIDEO: this->frame_video_fetching_func(); break;
      case MediaType::AUDIO: this->frame_audio_fetching_func(); break;
      default: throw std::runtime_error(fmt::format("[{}] Could not identify "
                                                    "media type", FUNCDINFO));
    }
  } catch (std::exception const& err) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->dispatch_exit(err.what());
  }
}

void MediaFetcher::frame_video_fetching_func() {
  const Dim2 def_outdim = bound_dims(this->mdec->get_width() * PAR_HEIGHT,
  this->mdec->get_height() * PAR_WIDTH,
  MAX_FRAME_WIDTH,
  MAX_FRAME_HEIGHT);

  const double avg_fts = this->mdec->get_avgfts(AVMEDIA_TYPE_VIDEO);
  VideoConverter vconv(def_outdim.width,
  def_outdim.height,
  AV_PIX_FMT_RGB24,
  this->mdec->get_width(),
  this->mdec->get_height(),
  this->mdec->get_pix_fmt());

  while (!this->should_exit()) {
    {
      std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
      while (!this->is_playing() && !this->should_exit()) {
        this->resume_cond.wait_for(resume_notify_lock, std::chrono::milliseconds(PAUSED_SLEEP_TIME_MS));
      }
    }

    {
      std::lock_guard<std::mutex> alter_mutex_lock(this->alter_mutex);
      if (this->req_dims) {
        Dim2 req_dims_bounded = bound_dims(
        this->mdec->get_width() * PAR_HEIGHT,
        this->mdec->get_height() * PAR_WIDTH,
        this->req_dims->width,
        this->req_dims->height);

        Dim2 out_dim = bound_dims(
        req_dims_bounded.width,
        req_dims_bounded.height,
        MAX_FRAME_WIDTH,
        MAX_FRAME_HEIGHT);
        
        vconv.reset_dst_size(out_dim.width, out_dim.height);
      }
    }
    
    double wait_duration = avg_fts;
    std::vector<AVFrame*> dec_frames;
    double current_time = 0.0;

    {
      std::lock_guard<std::mutex> mutex_lock(this->alter_mutex);
      current_time = this->get_time(sys_clk_sec());
      dec_frames = this->mdec->next_frames(AVMEDIA_TYPE_VIDEO);
    }

    if (dec_frames.size() > 0 && dec_frames[0] != nullptr) {
      const double frame_pts_time_sec = (double)dec_frames[0]->pts * this->mdec->get_time_base(AVMEDIA_TYPE_VIDEO);
      const double extra_delay = (double)(dec_frames[0]->repeat_pict) / (2 * avg_fts);
      wait_duration = frame_pts_time_sec - current_time + extra_delay;

      if (wait_duration > 0.0 || this->frame.get_width() * this->frame.get_height() == 0) {
        AVFrame* frame_image = vconv.convert_video_frame(dec_frames[0]);
        PixelData frame_pixel_data = PixelData(frame_image);
        {
          std::lock_guard<std::mutex> lock(this->alter_mutex);
          this->frame = frame_pixel_data;
        }
        av_frame_free(&frame_image);
      }
      clear_avframe_list(dec_frames);
      std::unique_lock<std::mutex> exit_lock(this->ex_noti_mtx);
      if (wait_duration > 0.0 && !this->should_exit()) {
        this->exit_cond.wait_for(exit_lock, secs_to_chns(wait_duration)); 
      }
    } else { // no frame was found.
      std::unique_lock<std::mutex> exit_lock(this->ex_noti_mtx);
      if (!this->should_exit()) {
        this->exit_cond.wait_for(exit_lock, secs_to_chns(DEFAULT_AVGFTS)); 
      }
    }

  }

}

void MediaFetcher::frame_image_fetching_func() {
  Dim2 outdim = bound_dims(this->mdec->get_width() * PAR_HEIGHT,
  this->mdec->get_height() * PAR_WIDTH,
  MAX_FRAME_WIDTH,
  MAX_FRAME_HEIGHT);

  VideoConverter vconv(outdim.width,
  outdim.height,
  AV_PIX_FMT_RGB24,
  this->mdec->get_width(),
  this->mdec->get_height(),
  this->mdec->get_pix_fmt());

  std::vector<AVFrame*> dec_frames;

  {
    std::lock_guard<std::mutex> mutex_lock(this->alter_mutex);
    dec_frames = this->mdec->next_frames(AVMEDIA_TYPE_VIDEO);
  }

  if (dec_frames.size() > 0 && dec_frames[0] != nullptr) {
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

  while (!this->should_exit()) {
    {
      std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
      while (!this->is_playing() && !this->should_exit()) {
        this->resume_cond.wait_for(resume_notify_lock, std::chrono::milliseconds(PAUSED_SLEEP_TIME_MS));
      }
    }

    Dim2 visdim(MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);

    {
      std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
      if (this->req_dims) {
        visdim = bound_dims(
        this->req_dims->width,
        this->req_dims->height,
        MAX_FRAME_WIDTH,
        MAX_FRAME_HEIGHT);
      }
    }

    if (this->audio_buffer->try_peek_into(aubduf_sz, audbuf, AUDIO_PEEK_TRY_WAIT_MS)) {
      std::unique_ptr<Visualizer> visualizer;
      {
        std::scoped_lock<std::mutex> alter_lock(this->alter_mutex);
        if (this->audio_visualizer) visualizer = this->audio_visualizer->clone();
      }

      if (visualizer) {
        PixelData frame = visualizer->visualize(audbuf, aubduf_sz, nb_ch, visdim.width, visdim.height);
        std::scoped_lock<std::mutex> alter_lock(this->alter_mutex);
        this->frame = frame;
      }
    }


    std::unique_lock<std::mutex> exit_lock(this->ex_noti_mtx);
    if (!this->should_exit()) {
      this->exit_cond.wait_for(exit_lock, secs_to_chns(DEFAULT_AVGFTS)); 
    }
  }
}
