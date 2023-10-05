
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <iostream>

#include "media.h"
#include "boiler.h"
#include "decode.h"
#include "sleep.h"
#include "icons.h"
#include "ascii.h"
#include "wmath.h"
#include "wtime.h"
#include "videoconverter.h"
#include "except.h"
#include "avguard.h"
#include "mediadecoder.h"

extern "C" {
#include <libavutil/version.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

const int MAX_FRAME_WIDTH = 27 * 16;
const int MAX_FRAME_HEIGHT = 27 * 9;
const char* DEBUG_VIDEO_SOURCE = "video";
const char* DEBUG_VIDEO_TYPE = "debug";


void MediaPlayer::video_playback_thread() {
  std::unique_lock<std::mutex> mutex_lock(this->alter_mutex, std::defer_lock);
  mutex_lock.lock();
  if (!this->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
    mutex_lock.unlock();
    throw std::runtime_error("Could not playback video data: Could not find video stream in media player");
  }

  const double avg_frame_time_sec = this->media_decoder->get_average_frame_time_sec(AVMEDIA_TYPE_VIDEO);
  const std::pair<int, int> bounded_video_frame_dimensions = get_bounded_dimensions(this->media_decoder->get_width(), this->media_decoder->get_height(), MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
  const int output_frame_width = bounded_video_frame_dimensions.first;
  const int output_frame_height = bounded_video_frame_dimensions.second;

  VideoConverter videoConverter(output_frame_width, output_frame_height, AV_PIX_FMT_RGB24, this->media_decoder->get_width(), this->media_decoder->get_height(), this->media_decoder->get_pix_fmt());
  mutex_lock.unlock();

  while (1) {
    mutex_lock.lock();
    if (!this->in_use)
    {
      mutex_lock.unlock();
      break;
    }

    if (!this->clock.is_playing()) {
      mutex_lock.unlock();
      sleep_quick();
      continue;
    }

    double frame_duration = avg_frame_time_sec;
    double frame_pts_time_sec = this->get_time(system_clock_sec()) + frame_duration;
    double extra_delay = 0.0;

    std::vector<AVFrame*> decoded_frames = this->media_decoder->next_frames(AVMEDIA_TYPE_VIDEO);

    if (decoded_frames.size() > 0 && decoded_frames[0] != nullptr) {
      mutex_lock.unlock();
      AVFrame* frame_image = videoConverter.convert_video_frame(decoded_frames[0]);
      mutex_lock.lock();
      this->set_current_frame(frame_image);

      #if HAS_AVFRAME_DURATION
      frame_duration = (double)frame_image->duration * this->media_decoder->get_time_base(AVMEDIA_TYPE_VIDEO);
      #endif
      frame_pts_time_sec = (double)frame_image->pts * this->media_decoder->get_time_base(AVMEDIA_TYPE_VIDEO);
      extra_delay = (double)(frame_image->repeat_pict) / (2 * avg_frame_time_sec);
      av_frame_free(&frame_image);
    }

    clear_av_frame_list(decoded_frames);


    const double frame_speed_skip_time_sec = ( frame_duration - ( frame_duration / this->clock.get_speed() ) );
    this->clock.skip(frame_speed_skip_time_sec);

    const double current_time = this->get_time(system_clock_sec());
    const double waitDuration = frame_pts_time_sec - current_time + extra_delay - frame_speed_skip_time_sec;

    mutex_lock.unlock();
    if (waitDuration <= 0) {
      continue;
    } else {
      sleep_for_sec(waitDuration);
    }
  }

}



