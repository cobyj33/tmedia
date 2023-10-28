
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
    this->audio_buffer = std::move(std::make_unique<AudioBuffer>(this->media_decoder->get_nb_channels(), this->media_decoder->get_sample_rate()));
  }
}

void MediaFetcher::dispatch_exit(std::string err) {
  if (err.length() == 0)
    throw std::runtime_error("[MediaFetcher::set_error] Cannot set error with empty string");
  this->error = err;
  this->dispatch_exit();
}

void MediaFetcher::dispatch_exit() {
  std::scoped_lock<std::mutex, std::mutex, std::mutex> notification_locks(this->exit_notify_mutex, this->audio_buffer_request_mutex, this->resume_notify_mutex);
  this->in_use = false;
  this->exit_cond.notify_all();
  this->audio_buffer_cond.notify_all();
  this->resume_cond.notify_all();
}

bool MediaFetcher::should_exit() {
  return !this->in_use;
}

bool MediaFetcher::is_playing() {
  if (this->media_type == MediaType::IMAGE)
    throw std::runtime_error("[MediaFetcher::is_playing] Cannot check playing state of Image media file");
  return this->clock.is_playing();
}

void MediaFetcher::pause(double current_system_time) {
  if (this->media_type == MediaType::IMAGE)
    throw std::runtime_error("[MediaFetcher::pause] Cannot pause image media file");
  this->clock.stop(current_system_time);
}

void MediaFetcher::resume(double current_system_time) {
  if (this->media_type == MediaType::IMAGE)
    throw std::runtime_error("[MediaFetcher::pause] Cannot resume image media file");
  this->clock.resume(current_system_time);
  std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
  this->resume_cond.notify_all();
}

bool MediaFetcher::has_error() const noexcept {
  return this->error.has_value();
}

std::string MediaFetcher::get_error() {
  if (!this->error.has_value())
    throw std::runtime_error("[MediaFetcher::get_error] Cannot get error when no error is available");
  return *this->error;
}

/**
 * Thread-Safe
*/
bool MediaFetcher::has_media_stream(enum AVMediaType media_type) const {
  return this->media_decoder->has_media_decoder(media_type);
}

/**
 * Thread-Safe
*/
double MediaFetcher::get_duration() const {
  return this->media_decoder->get_duration();
}

/**
 * For thread safety, alter_mutex must be locked
*/
double MediaFetcher::get_time(double current_system_time) const {
  return this->clock.get_time(current_system_time);
}

/**
 * For threadsafety, both alter_mutex and buffer_read_mutex must be locked
*/
double MediaFetcher::get_desync_time(double current_system_time) const {
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO) && this->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
    double current_playback_time = this->get_time(current_system_time);
    double desync = std::abs(this->audio_buffer->get_time() - current_playback_time); // in our implementation, only the audio buffer can really get desynced from our MediaClock
    return desync;
  }
  return 0.0; // if there is only one stream, there cannot be desync 
}

/**
 * Convencience method for loading audio.
 * 
 * Requires both the alter_mutex and the audio_buffer_mutex to be locked
*/
int MediaFetcher::load_next_audio() {
  if (!this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error("[MediaFetcher::load_next_audio_frames] Cannot load "
    "next audio frames, Media Fetcher does not have any audio stream to load data from");
  }

  AudioResampler audio_resampler(
  this->media_decoder->get_ch_layout(), AV_SAMPLE_FMT_FLT, this->media_decoder->get_sample_rate(),
  this->media_decoder->get_ch_layout(), this->media_decoder->get_sample_fmt(), this->media_decoder->get_sample_rate());

  int written_samples = 0;
  std::vector<AVFrame*> next_raw_audio_frames = this->media_decoder->next_frames(AVMEDIA_TYPE_AUDIO);
  std::vector<AVFrame*> audio_frames = audio_resampler.resample_audio_frames(next_raw_audio_frames);
  for (int i = 0; i < (int)audio_frames.size(); i++) {
    this->audio_buffer->write((float*)(audio_frames[i]->data[0]), audio_frames[i]->nb_samples);
    written_samples += audio_frames[i]->nb_samples;
  }

  clear_av_frame_list(next_raw_audio_frames);
  clear_av_frame_list(audio_frames);

  return written_samples;
}

/**
 * For threadsafety, both alter_mutex and buffer_read_mutex must be locked
*/
int MediaFetcher::jump_to_time(double target_time, double current_system_time) {
  
  if (target_time < 0.0 || target_time > this->get_duration()) {
    throw std::runtime_error("Could not jump to time " + format_time_hh_mm_ss(target_time) + " ( " + std::to_string(target_time) + " seconds ) "
    ", time is out of the bounds of duration " + format_time_hh_mm_ss(target_time) + " ( " + std::to_string(this->get_duration()) + " seconds )");
  }

  const double original_time = this->get_time(current_system_time);
  int ret = this->media_decoder->jump_to_time(target_time);

  if (ret < 0)
    return ret;
  
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) { // refilling on time jumps helps prevent playback crackling when the audio buffer gets cleared
    const int AUDIO_BUFFER_MAXIMUM_REFILL_FRAME_SIZE = 16384;
    const int MAX_LOAD_ITERATIONS = 5; 
    this->audio_buffer->clear_and_restart_at(target_time);

    int total_written = 0;
    for (int i = 0; i < MAX_LOAD_ITERATIONS && total_written < AUDIO_BUFFER_MAXIMUM_REFILL_FRAME_SIZE; i++) {
      int samples_written = this->load_next_audio();
      if (samples_written == 0) break; // eof
      total_written += samples_written;
    }
  }
  
  this->clock.skip(target_time - original_time); // Update the playback to account for the skipped time
  std::unique_lock<std::mutex> audio_buffer_request_lock(this->audio_buffer_request_mutex);
  this->audio_buffer_cond.notify_one();
  return ret;
}

/**
 * Cannot be called inside video_fetching_thread or audio_fetching_thread at all
 * 
 * Writes some audio to the audio buffer initially, and starts the audio and video threads
 * 
 * Note that if the user should lock the audio_buffer_mutex before calling this function
 * if another thread is already attempting to read from the audio buffer
*/
void MediaFetcher::begin() {
  this->in_use = true;

  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO))
    for (int i = 0; i < 10; i++)
      this->load_next_audio();

  this->clock.init(system_clock_sec());

  std::thread initialized_duration_checking_thread(&MediaFetcher::duration_checking_thread_func, this);
  duration_checking_thread.swap(initialized_duration_checking_thread);
  std::thread initialized_video_thread(&MediaFetcher::video_fetching_thread_func, this);
  video_thread.swap(initialized_video_thread);
  std::thread initialized_audio_thread(&MediaFetcher::audio_dispatch_thread_func, this);
  audio_thread.swap(initialized_audio_thread);
}

/**
 * Cannot be called inside video_fetching_thread or audio_fetching_thread at all
*/
void MediaFetcher::join() {
  this->in_use = false; // the user can set this as well if they want, but this is to make sure that the threads WILL end regardless
  if (this->media_type != MediaType::IMAGE && this->is_playing()) this->pause(system_clock_sec());
  this->video_thread.join();
  this->duration_checking_thread.join();
  this->audio_thread.join();
}

