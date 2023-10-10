
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <string>
#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <deque>

#include "boiler.h"
#include "media.h"
#include "wtime.h"
#include "wmath.h"
#include "except.h"
#include "formatting.h"
#include "sleep.h"
#include "audioresampler.h"
#include "avguard.h"
#include "streamdecoder.h"

extern "C" {
#include "curses.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

MediaPlayer::MediaPlayer(const char* file_name, MediaGUI starting_media_gui) {
  this->in_use = false;
  this->file_name = file_name;
  this->is_looped = false;
  this->muted = false;
  this->media_gui = starting_media_gui;
  this->volume = 1.0;

  std::vector<enum AVMediaType> media_types = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
  this->media_decoder = std::move(std::make_unique<MediaDecoder>(file_name));
  this->media_type = this->media_decoder->get_media_type();


  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    this->audio_resampler = std::move(std::make_unique<AudioResampler>(
    this->media_decoder->get_ch_layout(), AV_SAMPLE_FMT_FLT, this->media_decoder->get_sample_rate(),
    this->media_decoder->get_ch_layout(), this->media_decoder->get_sample_fmt(), this->media_decoder->get_sample_rate()));
    this->audio_buffer = std::move(std::make_unique<AudioBuffer>(this->media_decoder->get_nb_channels(), this->media_decoder->get_sample_rate()));
  }
}

void MediaPlayer::set_current_frame(PixelData& data) {
  this->frame = data;
}

void MediaPlayer::set_current_frame(AVFrame* frame) {
  this->frame = frame;
}

PixelData& MediaPlayer::get_current_frame() {
  return this->frame;
}

bool MediaPlayer::has_media_stream(enum AVMediaType media_type) const {
  return this->media_decoder->has_media_decoder(media_type);
}

double MediaPlayer::get_duration() const {
  return this->media_decoder->get_duration();
}

double MediaPlayer::get_time(double current_system_time) const {
  return this->clock.get_time(current_system_time);
}

bool MediaPlayer::has_error() {
  return this->error.length() > 0;
}

std::string MediaPlayer::get_error() {
  return this->error;
}


void MediaPlayer::start(double start_time) {
  if (this->in_use) {
    throw std::runtime_error("[MediaPlayer::start] Cannot use a MediaPlayer "
    "that is already in use");
  }
  if (start_time < 0) {
    throw std::runtime_error("[MediaPlayer::start] Cannot start a MediaPlayer "
    "at a negative time ( got " + std::to_string(start_time) +  ")");
  }
  if (start_time > this->get_duration()) {
    throw std::runtime_error("[MediaPlayer::start] Cannot start a MediaPlayer at"
    "a time greater than the stream duration ( got " + std::to_string(start_time) +  
    " seconds. Stream ends at " + std::to_string(this->get_duration()) + " seconds)");
  }

  this->in_use = true;

  if (this->media_type == MediaType::IMAGE) {
    this->frame = PixelData(this->file_name);
  } else if (this->media_type == MediaType::VIDEO || this->media_type == MediaType::AUDIO) {
    this->clock.start(system_clock_sec());
    if (start_time != 0) {
      this->jump_to_time(start_time, system_clock_sec());
    }
  }

  std::thread audio_thread;
  bool audio_thread_initialized = false;

  std::thread video_thread(&MediaPlayer::video_playback_thread, this);
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    std::thread initialized_audio_thread(&MediaPlayer::audio_playback_thread, this);
    audio_thread.swap(initialized_audio_thread);
    audio_thread_initialized = true;
  }

  this->render_loop();
  video_thread.join();
  if (audio_thread_initialized) {
    audio_thread.join();
  }

  if (this->clock.is_playing()) {
    this->clock.stop(system_clock_sec());
  }
  this->in_use = false;
}

double MediaPlayer::get_desync_time(double current_system_time) const {
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO) && this->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
    double current_playback_time = this->get_time(current_system_time);
    double desync = std::abs(this->audio_buffer->get_time() - current_playback_time); // in our implementation, only the audio buffer can really get desynced from our MediaClock
    return desync;
  }
  return 0.0; // if there is only one stream, there cannot be desync 
}


int MediaPlayer::load_next_audio() {
  if (!this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error("[MediaPlayer::load_next_audio_frames] Cannot load "
    "next audio frames, Media Player does not have any audio stream to load data from");
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

int MediaPlayer::jump_to_time(double target_time, double current_system_time) {
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


