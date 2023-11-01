#include "mediafetcher.h"

#include "termenv.h"
#include "decode.h"
#include "audio.h"
#include "audio_image.h"
#include "sleep.h"
#include "scale.h"
#include "wtime.h"
#include "videoconverter.h"

#include <mutex>
#include <memory>
#include <stdexcept>
#include <chrono>

extern "C" {
#include <libavutil/version.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

/**
 * The purpose of the video thread is to update the current MediaFetcher's frame for rendering
 * 
 * This is done in different ways depending on if a video is available in the current media type or not
 * If the currently attached media is a video:
 *  The video thread will update the current frame to be the correct frame according to the MediaFetcher's current
 *  playback timestamp
 * 
 * If the currently attached media is audio:
 *  The video thread will update the current frame to be a snapshot of the wave of audio data currently being
 *  processed. 
*/

const int PIXEL_ASPECT_RATIO_WIDTH = 2; // account for non-square shape of terminal characters
const int PIXEL_ASPECT_RATIO_HEIGHT = 5;

const int MAX_FRAME_ASPECT_RATIO_WIDTH = 16 * PIXEL_ASPECT_RATIO_HEIGHT;
const int MAX_FRAME_ASPECT_RATIO_HEIGHT = 9 * PIXEL_ASPECT_RATIO_WIDTH;
const double MAX_FRAME_ASPECT_RATIO = (double)MAX_FRAME_ASPECT_RATIO_WIDTH / (double)MAX_FRAME_ASPECT_RATIO_HEIGHT;

const int MAX_FRAME_WIDTH = 640;
const int MAX_FRAME_HEIGHT = MAX_FRAME_WIDTH / MAX_FRAME_ASPECT_RATIO;
const int PAUSED_SLEEP_TIME_MS = 500;
  const double DEFAULT_AVERAGE_FRAME_TIME_SEC = 1.0 / 24.0;

void MediaFetcher::video_fetching_thread_func() {
  try {
    switch (this->media_type) {
      case MediaType::IMAGE: this->frame_image_fetching_func(); break;
      case MediaType::VIDEO: this->frame_video_fetching_func(); break;
      case MediaType::AUDIO: this->frame_audio_fetching_func(); break;
      default: throw std::runtime_error("[MediaFetcher::video_fetching_thread] Could not identify media type");
    }
  } catch (std::exception const& err) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->dispatch_exit(err.what());
  }
}

void MediaFetcher::frame_video_fetching_func() {
  const VideoDimensions default_output_frame_dim = get_bounded_dimensions(this->media_decoder->get_width() * PIXEL_ASPECT_RATIO_HEIGHT,
  this->media_decoder->get_height() * PIXEL_ASPECT_RATIO_WIDTH,
  MAX_FRAME_WIDTH,
  MAX_FRAME_HEIGHT);

  const double avg_frame_time_sec = this->media_decoder->get_average_frame_time_sec(AVMEDIA_TYPE_VIDEO);
  VideoConverter video_converter(default_output_frame_dim.width,
  default_output_frame_dim.height,
  AV_PIX_FMT_RGB24,
  this->media_decoder->get_width(),
  this->media_decoder->get_height(),
  this->media_decoder->get_pix_fmt());

  while (!this->should_exit()) {
    {
      std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
      while (!this->is_playing() && !this->should_exit()) {
        this->resume_cond.wait_for(resume_notify_lock, std::chrono::milliseconds(PAUSED_SLEEP_TIME_MS));
      }
    }

    {
      std::lock_guard<std::mutex> alter_mutex_lock(this->alter_mutex);
      if (this->requested_frame_dims) {
        VideoDimensions bounded_requested_dims = get_bounded_dimensions(
        this->requested_frame_dims->width * PIXEL_ASPECT_RATIO_HEIGHT,
        this->requested_frame_dims->height * PIXEL_ASPECT_RATIO_WIDTH,
        MAX_FRAME_WIDTH,
        MAX_FRAME_HEIGHT);

        VideoDimensions output_frame_dim = get_bounded_dimensions(
        this->media_decoder->get_width() * PIXEL_ASPECT_RATIO_HEIGHT,
        this->media_decoder->get_height() * PIXEL_ASPECT_RATIO_WIDTH,
        bounded_requested_dims.width,
        bounded_requested_dims.height);

        video_converter.reset_dst_size(output_frame_dim.width, output_frame_dim.height);
      }
    }
    
    double wait_duration = avg_frame_time_sec;
    std::vector<AVFrame*> decoded_frames;
    double current_time = 0.0;

    {
      std::lock_guard<std::mutex> mutex_lock(this->alter_mutex);
      current_time = this->get_time(system_clock_sec());
      decoded_frames = this->media_decoder->next_frames(AVMEDIA_TYPE_VIDEO);
    }

    if (decoded_frames.size() > 0 && decoded_frames[0] != nullptr) {
      const double frame_pts_time_sec = (double)decoded_frames[0]->pts * this->media_decoder->get_time_base(AVMEDIA_TYPE_VIDEO);
      const double extra_delay = (double)(decoded_frames[0]->repeat_pict) / (2 * avg_frame_time_sec);
      wait_duration = frame_pts_time_sec - current_time + extra_delay;

      if (wait_duration > 0.0 || this->frame.get_width() * this->frame.get_height() == 0) {
        AVFrame* frame_image = video_converter.convert_video_frame(decoded_frames[0]);
        PixelData frame_pixel_data = PixelData(frame_image);
        {
          std::lock_guard<std::mutex> lock(this->alter_mutex);
          this->frame = frame_pixel_data;
        }
        av_frame_free(&frame_image);
      }
      clear_av_frame_list(decoded_frames);
      std::unique_lock<std::mutex> exit_lock(this->exit_notify_mutex);
      if (wait_duration > 0.0 && !this->should_exit()) {
        this->exit_cond.wait_for(exit_lock, seconds_to_chrono_nanoseconds(wait_duration)); 
      }
    } else { // no frame was found.
      std::unique_lock<std::mutex> exit_lock(this->exit_notify_mutex);
      if (!this->should_exit()) {
        this->exit_cond.wait_for(exit_lock, seconds_to_chrono_nanoseconds(DEFAULT_AVERAGE_FRAME_TIME_SEC)); 
      }
    }

  }

}

void MediaFetcher::frame_image_fetching_func() {
  VideoDimensions output_frame_dim = get_bounded_dimensions(this->media_decoder->get_width() * PIXEL_ASPECT_RATIO_HEIGHT,
  this->media_decoder->get_height() * PIXEL_ASPECT_RATIO_WIDTH,
  MAX_FRAME_WIDTH,
  MAX_FRAME_HEIGHT);

  VideoConverter video_converter(output_frame_dim.width,
  output_frame_dim.height,
  AV_PIX_FMT_RGB24,
  this->media_decoder->get_width(),
  this->media_decoder->get_height(),
  this->media_decoder->get_pix_fmt());

  std::vector<AVFrame*> decoded_frames;

  {
    std::lock_guard<std::mutex> mutex_lock(this->alter_mutex);
    decoded_frames = this->media_decoder->next_frames(AVMEDIA_TYPE_VIDEO);
  }

  if (decoded_frames.size() > 0 && decoded_frames[0] != nullptr) {
    AVFrame* frame_image = video_converter.convert_video_frame(decoded_frames[0]);
    PixelData frame_pixel_data = PixelData(frame_image);
    {
      std::lock_guard<std::mutex> lock(this->alter_mutex);
      this->frame = frame_pixel_data;
    }
    av_frame_free(&frame_image);
  }

  clear_av_frame_list(decoded_frames);
}

void MediaFetcher::frame_audio_fetching_func() {
  static constexpr int AUDIO_MAX_PEEK_SIZE = 44100 / 2;

  while (!this->should_exit()) {
    {
      std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
      while (!this->is_playing() && !this->should_exit()) {
        this->resume_cond.wait_for(resume_notify_lock, std::chrono::milliseconds(PAUSED_SLEEP_TIME_MS));
      }
    }

    
    std::vector<float> audio_buffer_view;
    int nb_channels = this->audio_buffer->get_nb_channels();
    VideoDimensions audio_frame_dims(MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);

    {
      std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
      if (this->requested_frame_dims) {
        audio_frame_dims = get_bounded_dimensions(
        this->requested_frame_dims->width,
        this->requested_frame_dims->height,
        MAX_FRAME_WIDTH,
        MAX_FRAME_HEIGHT);
      }
    }

    {
      std::lock_guard<std::mutex> buffer_read_lock(this->audio_buffer_mutex);
      const int peek_size = std::min(this->audio_buffer->get_nb_can_read(), AUDIO_MAX_PEEK_SIZE);
      if (peek_size > 0) {
        audio_buffer_view = this->audio_buffer->peek_into(peek_size);
      }
    }

    if (audio_buffer_view.size() > 0) {
    std::vector<float> mono = audio_to_mono(audio_buffer_view, nb_channels);
    audio_bound_volume(mono, 1, 1.0);
    PixelData audio_visualization = generate_audio_view_amplitude_averaged(mono, audio_frame_dims.height, audio_frame_dims.width);

      std::lock_guard<std::mutex> player_lock(this->alter_mutex);
      this->frame = audio_visualization;
    }


    std::unique_lock<std::mutex> exit_lock(this->exit_notify_mutex);
    if (!this->should_exit()) {
      this->exit_cond.wait_for(exit_lock, seconds_to_chrono_nanoseconds(DEFAULT_AVERAGE_FRAME_TIME_SEC)); 
    }
  }
}
