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



void MediaFetcher::video_fetching_thread() {
  const int PIXEL_ASPECT_RATIO_WIDTH = 2; // account for non-square shape of terminal characters
  const int PIXEL_ASPECT_RATIO_HEIGHT = 5;

  const int MAX_FRAME_ASPECT_RATIO_WIDTH = 16 * PIXEL_ASPECT_RATIO_HEIGHT;
  const int MAX_FRAME_ASPECT_RATIO_HEIGHT = 9 * PIXEL_ASPECT_RATIO_WIDTH;
  const double MAX_FRAME_ASPECT_RATIO = (double)MAX_FRAME_ASPECT_RATIO_WIDTH / (double)MAX_FRAME_ASPECT_RATIO_HEIGHT;

  const int MAX_FRAME_WIDTH = 640;
  const int MAX_FRAME_HEIGHT = MAX_FRAME_WIDTH / MAX_FRAME_ASPECT_RATIO;

  const double DEFAULT_AVERAGE_FRAME_TIME_SEC = 1.0 / 24.0;

  try {
    double avg_frame_time_sec = DEFAULT_AVERAGE_FRAME_TIME_SEC;

    switch (this->media_type) {
      case MediaType::IMAGE:
      case MediaType::VIDEO: {

        const std::pair<int, int> bounded_video_frame_dimensions = get_bounded_dimensions(this->media_decoder->get_width() * PIXEL_ASPECT_RATIO_HEIGHT, this->media_decoder->get_height() * PIXEL_ASPECT_RATIO_WIDTH, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
        const int output_frame_width = bounded_video_frame_dimensions.first;
        const int output_frame_height = bounded_video_frame_dimensions.second;

        avg_frame_time_sec = this->media_decoder->get_average_frame_time_sec(AVMEDIA_TYPE_VIDEO);
        VideoConverter video_converter(output_frame_width, output_frame_height, AV_PIX_FMT_RGB24, this->media_decoder->get_width(), this->media_decoder->get_height(), this->media_decoder->get_pix_fmt());

        while (this->in_use) {
          if (!this->clock.is_playing()) {
            sleep_for_ms(17);
            continue;
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
            // resizing can take quite some time, so we release the mutex for this part
            const double frame_pts_time_sec = (double)decoded_frames[0]->pts * this->media_decoder->get_time_base(AVMEDIA_TYPE_VIDEO);
            const double extra_delay = (double)(decoded_frames[0]->repeat_pict) / (2 * avg_frame_time_sec);
            wait_duration = frame_pts_time_sec - current_time + extra_delay;

            if (wait_duration > 0.0) {
              AVFrame* frame_image = video_converter.convert_video_frame(decoded_frames[0]);
              PixelData frame_pixel_data = PixelData(frame_image);
              {
                std::lock_guard<std::mutex> lock(this->alter_mutex);
                this->frame = frame_pixel_data;
              }
              av_frame_free(&frame_image);
            }
          }

          clear_av_frame_list(decoded_frames);

          if (this->media_type == MediaType::IMAGE) break;
          if (wait_duration > 0.0) {
            sleep_for_sec(wait_duration);
          }
        }

      } break;
      case MediaType::AUDIO: {

        while (this->in_use) {
          const std::size_t AUDIO_PEEK_SIZE = 44100 * 2;
          
          std::vector<float> audio_buffer_view;
          int nb_channels;

          {
            std::lock_guard<std::mutex> buffer_read_lock(this->audio_buffer_mutex);

            std::size_t to_peek = std::min(this->audio_buffer->get_nb_can_read(), AUDIO_PEEK_SIZE);
            if (to_peek == 0) {
              continue;
            }
            audio_buffer_view = this->audio_buffer->peek_into(to_peek);
            nb_channels = this->audio_buffer->get_nb_channels();
          }

          std::vector<float> mono = audio_to_mono(audio_buffer_view, nb_channels);
          audio_bound_volume(mono, 1, 1.0);
          PixelData audio_visualization = generate_audio_view_amplitude_averaged(mono, MAX_FRAME_HEIGHT, MAX_FRAME_WIDTH);

          {
            std::lock_guard<std::mutex> player_lock(this->alter_mutex);
            this->frame = audio_visualization;
          }

          sleep_for_sec(DEFAULT_AVERAGE_FRAME_TIME_SEC);
        }

      } break;
      default: throw std::runtime_error("[MediaFetcher::video_fetching_thread] Could not identify media type");
    }

  } catch (std::exception const& e) {
    std::lock_guard<std::mutex> lock(this->alter_mutex);
    this->error = std::string(e.what());
    this->in_use = false;
  }
}



