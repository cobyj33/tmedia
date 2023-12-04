
#include "mediafetcher.h"

#include "decode.h"
#include "wtime.h"
#include "wmath.h"
#include "formatting.h"
#include "audioresampler.h"
#include "audio_visualizer.h"

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
  this->audio_visualizer = std::move(std::make_unique<AmplitudeAbs>());


  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    static constexpr int INTERNAL_AUDIO_BUFFER_LENGTH_SECONDS = 5;
    const int sample_rate = this->media_decoder->get_sample_rate();
    const int nb_channels = this->media_decoder->get_nb_channels();
    const int frame_capacity = sample_rate * INTERNAL_AUDIO_BUFFER_LENGTH_SECONDS;
    this->audio_buffer = std::move(std::make_unique<BlockingAudioRingBuffer>(frame_capacity, nb_channels, sample_rate, 0.0));
  }
}

void MediaFetcher::dispatch_exit(std::string err) {
  if (err.length() == 0)
    throw std::runtime_error("[MediaFetcher::set_error] Cannot set error with empty string");
  this->error = err;
  this->dispatch_exit();
}

void MediaFetcher::dispatch_exit() {
  std::scoped_lock<std::mutex, std::mutex> notification_locks(this->exit_notify_mutex, this->resume_notify_mutex);
  this->in_use = false;
  this->exit_cond.notify_all();
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
  return this->media_decoder->has_stream_decoder(media_type);
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
 * For threadsafety, alter_mutex must be locked
*/
double MediaFetcher::get_desync_time(double current_system_time) const {
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    double playback_time = this->get_time(current_system_time);
    double audio_time = this->audio_buffer->get_buffer_current_time();
    return std::abs(audio_time - playback_time);
  }
  return 0.0; // Video doesn't really get desynced since the video thread syncs itself to the MediaClock
}

/**
 * For threadsafety, both alter_mutex must be locked
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
  
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    this->audio_buffer->clear(target_time);
  }
  
  this->clock.skip(target_time - original_time); // Update the playback to account for the skipped time
  return ret;
}

void MediaFetcher::begin(double current_system_time) {
  this->in_use = true;
  this->clock.init(current_system_time);

  std::thread initialized_duration_checking_thread(&MediaFetcher::duration_checking_thread_func, this);
  duration_checking_thread.swap(initialized_duration_checking_thread);
  std::thread initialized_video_thread(&MediaFetcher::video_fetching_thread_func, this);
  video_thread.swap(initialized_video_thread);
  std::thread initialized_audio_thread(&MediaFetcher::audio_dispatch_thread_func, this);
  audio_thread.swap(initialized_audio_thread);
}

void MediaFetcher::join(double current_system_time) {
  this->in_use = false; // the user can set this as well if they want, but this is to make sure that the threads WILL end regardless
  if (this->media_type != MediaType::IMAGE && this->is_playing()) this->pause(current_system_time);
  this->video_thread.join();
  this->duration_checking_thread.join();
  this->audio_thread.join();
}

