
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <iostream>

#include "mediaplayer.h"
#include "boiler.h"
#include "decode.h"
#include "sleep.h"
#include "ascii.h"
#include "wmath.h"
#include "wtime.h"
#include "videoconverter.h"
#include "except.h"
#include "avguard.h"
#include "mediadecoder.h"

#include "audio_image.h"
#include "audio.h"

extern "C" {
#include <libavutil/version.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

const int MAX_FRAME_WIDTH = 27 * 16;
const int MAX_FRAME_HEIGHT = 27 * 9;

/**
 * The purpose of the video thread is to update the current MediaPlayer's frame for rendering
 * 
 * This is done in different ways depending on if a video is available in the current media type or not
 * If the currently attached media is a video:
 *  The video thread will update the current frame to be the correct frame according to the MediaPlayer's current
 *  playback timestamp
 * 
 * If the currently attached media is audio:
 *  The video thread will update the current frame to be a snapshot of the wave of audio data currently being
 *  processed. 
*/


void MediaPlayer::video_playback_thread() {

  try {
    std::unique_ptr<VideoConverter> video_converter;
    double avg_frame_time_sec = 1 / 24.0;

    if (this->has_media_stream(AVMEDIA_TYPE_VIDEO)) {

        {
          std::lock_guard<std::mutex> mutex_lock(this->alter_mutex);

          const std::pair<int, int> bounded_video_frame_dimensions = get_bounded_dimensions(this->media_decoder->get_width(), this->media_decoder->get_height(), MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
          const int output_frame_width = bounded_video_frame_dimensions.first;
          const int output_frame_height = bounded_video_frame_dimensions.second;

          avg_frame_time_sec = this->media_decoder->get_average_frame_time_sec(AVMEDIA_TYPE_VIDEO);
          video_converter = std::move(std::make_unique<VideoConverter>(output_frame_width, output_frame_height, AV_PIX_FMT_RGB24, this->media_decoder->get_width(), this->media_decoder->get_height(), this->media_decoder->get_pix_fmt()));
        }

        while (this->in_use) {
          double wait_duration = 0.0;
          std::vector<AVFrame*> decoded_frames;

          {
            std::lock_guard<std::mutex> mutex_lock(this->alter_mutex);
            if (!this->clock.is_playing()) {
              continue;
            }

            decoded_frames = this->media_decoder->next_frames(AVMEDIA_TYPE_VIDEO);
          }

          if (decoded_frames.size() > 0 && decoded_frames[0] != nullptr) {
            // resizing can take quite some time, so we release the mutex for this part
            AVFrame* frame_image = video_converter->convert_video_frame(decoded_frames[0]);

            {
              std::lock_guard<std::mutex> lock(this->alter_mutex);
              this->set_current_frame(frame_image);

              #if HAS_AVFRAME_DURATION
              const double frame_duration = (double)frame_image->duration * this->media_decoder->get_time_base(AVMEDIA_TYPE_VIDEO);
              #else
              const double frame_duration = avg_frame_time_sec;
              #endif

              const double frame_pts_time_sec = (double)frame_image->pts * this->media_decoder->get_time_base(AVMEDIA_TYPE_VIDEO);
              const double extra_delay = (double)(frame_image->repeat_pict) / (2 * avg_frame_time_sec);
              const double frame_speed_skip_time_sec = ( frame_duration - ( frame_duration / this->clock.get_speed() ) );
              this->clock.skip(frame_speed_skip_time_sec);

              const double current_time = this->get_time(system_clock_sec());
              wait_duration = frame_pts_time_sec - current_time + extra_delay - frame_speed_skip_time_sec;
            }

            av_frame_free(&frame_image);
          } 

          clear_av_frame_list(decoded_frames);
          if (wait_duration <= 0) {
            continue;
          } else {
            sleep_for_sec(wait_duration);
          }
        }
    } else if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {

      while (this->in_use) {
        const int AUDIO_VIEW_ROWS = this->display_lines;
        const int AUDIO_VIEW_COLS = std::max(static_cast<int>(this->display_cols), 144);
        
        std::vector<float> audio_buffer_view;
        int nb_channels;

        {
          std::lock_guard<std::mutex> player_lock(this->alter_mutex);
          std::lock_guard<std::mutex> buffer_read_lock(this->buffer_read_mutex);

          std::size_t to_peek = std::min(this->audio_buffer->get_nb_can_read(), (std::size_t)AUDIO_VIEW_COLS);
          if (to_peek == 0) {
            continue;
          }
          audio_buffer_view = this->audio_buffer->peek_into(to_peek);
          nb_channels = this->audio_buffer->get_nb_channels();
        }

        std::vector<float> mono = audio_to_mono(audio_buffer_view, nb_channels);
        PixelData frame = generate_audio_view(mono, AUDIO_VIEW_ROWS, AUDIO_VIEW_COLS);

        {
          std::lock_guard<std::mutex> player_lock(this->alter_mutex);
          this->frame = frame;
        }

        sleep_for_ms(avg_frame_time_sec);
      }

    }

  } catch (std::exception const& e) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->error = std::string(e.what());
    this->in_use = false;
  }
}



