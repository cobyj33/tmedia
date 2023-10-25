
#include "mediafetcher.h"

#include "decode.h"
#include "wtime.h"
#include "wmath.h"
#include "formatting.h"
#include "audioresampler.h"

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <set>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

MediaFetcher::MediaFetcher(const std::string& file_path) {
  this->file_path = file_path;
  this->in_use = false;

  std::set<enum AVMediaType> requested_stream_types = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
  this->media_decoder = std::move(std::make_unique<MediaDecoder>(file_path, requested_stream_types));
  this->media_type = this->media_decoder->get_media_type();


  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    this->audio_resampler = std::move(std::make_unique<AudioResampler>(
    this->media_decoder->get_ch_layout(), AV_SAMPLE_FMT_FLT, this->media_decoder->get_sample_rate(),
    this->media_decoder->get_ch_layout(), this->media_decoder->get_sample_fmt(), this->media_decoder->get_sample_rate()));
    this->audio_buffer = std::move(std::make_unique<AudioBuffer>(this->media_decoder->get_nb_channels(), this->media_decoder->get_sample_rate()));
  }
}

bool MediaFetcher::has_media_stream(enum AVMediaType media_type) const {
  return this->media_decoder->has_media_decoder(media_type);
}

double MediaFetcher::get_duration() const {
  return this->media_decoder->get_duration();
}

double MediaFetcher::get_time(double current_system_time) const {
  return this->clock.get_time(current_system_time);
}

double MediaFetcher::get_desync_time(double current_system_time) const {
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO) && this->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
    double current_playback_time = this->get_time(current_system_time);
    double desync = std::abs(this->audio_buffer->get_time() - current_playback_time); // in our implementation, only the audio buffer can really get desynced from our MediaClock
    return desync;
  }
  return 0.0; // if there is only one stream, there cannot be desync 
}


int MediaFetcher::load_next_audio() {
  if (!this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error("[MediaFetcher::load_next_audio_frames] Cannot load "
    "next audio frames, Media Fetcher does not have any audio stream to load data from");
  }

  int written_samples = 0;
  std::vector<AVFrame*> next_raw_audio_frames = this->media_decoder->next_frames(AVMEDIA_TYPE_AUDIO);
  std::vector<AVFrame*> audio_frames = this->audio_resampler->resample_audio_frames(next_raw_audio_frames);
  for (int i = 0; i < (int)audio_frames.size(); i++) {
    this->audio_buffer->write((float*)(audio_frames[i]->data[0]), audio_frames[i]->nb_samples);
    written_samples += audio_frames[i]->nb_samples;
  }

  clear_av_frame_list(next_raw_audio_frames);
  clear_av_frame_list(audio_frames);

  return written_samples;
}

int MediaFetcher::jump_to_time(double target_time, double current_system_time) {
  if (target_time < 0.0 || target_time > this->get_duration()) {
    throw std::runtime_error("Could not jump to time " + format_time_hh_mm_ss(target_time) + " ( " + std::to_string(target_time) + " seconds ) "
    ", time is out of the bounds of duration " + format_time_hh_mm_ss(target_time) + " ( " + std::to_string(this->get_duration()) + " seconds )");
  }

  const double original_time = this->get_time(current_system_time);
  int ret = this->media_decoder->jump_to_time(target_time);

  if (ret < 0)
    return ret;
  
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    this->audio_buffer->clear_and_restart_at(target_time);
  }
  
  this->clock.skip(target_time - original_time); // Update the playback to account for the skipped time
  return ret;
}

void MediaFetcher::begin() {
  this->in_use = true;
  this->clock.init(system_clock_sec());
  std::thread initialized_video_thread(&MediaFetcher::video_fetching_thread, this);
  video_thread.swap(initialized_video_thread);
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    std::thread initialized_audio_thread(&MediaFetcher::audio_fetching_thread, this);
    audio_thread.swap(initialized_audio_thread);
  }
}

void MediaFetcher::join() {
  this->video_thread.join();
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    this->audio_thread.join();
  }
  this->clock.stop(system_clock_sec());
}