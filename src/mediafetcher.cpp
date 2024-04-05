
#include "mediafetcher.h"

#include "decode.h"
#include "wtime.h"
#include "wmath.h"
#include "formatting.h"
#include "audioresampler.h"
#include "audio_visualizer.h"
#include "funcmac.h"

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <set>

#include <fmt/format.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

MediaFetcher::MediaFetcher(std::filesystem::path path) {
  this->path = path;
  this->in_use = false;

  std::set<enum AVMediaType> requested_stream_types = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
  this->mdec = std::move(std::make_unique<MediaDecoder>(path, requested_stream_types));
  this->media_type = this->mdec->get_media_type();
  this->audio_visualizer = std::move(std::make_unique<AmplitudeAbs>());


  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    static constexpr int INTERNAL_AUDIO_BUFFER_LENGTH_SECONDS = 5;
    const int sample_rate = this->mdec->get_sample_rate();
    const int nb_channels = this->mdec->get_nb_channels();
    const int frame_capacity = sample_rate * INTERNAL_AUDIO_BUFFER_LENGTH_SECONDS;
    this->audio_buffer = std::move(std::make_unique<BlockingAudioRingBuffer>(frame_capacity, nb_channels, sample_rate, 0.0));
  }
}

void MediaFetcher::dispatch_exit(std::string err) {
  if (err.length() == 0)
    throw std::runtime_error(fmt::format("[{}] Cannot set error "
    "with empty string", FUNCDINFO));
  this->error = err;
  this->dispatch_exit();
}

void MediaFetcher::dispatch_exit() {
  std::scoped_lock<std::mutex, std::mutex> notification_locks(this->ex_noti_mtx, this->resume_notify_mutex);
  this->in_use = false;
  this->exit_cond.notify_all();
  this->resume_cond.notify_all();
}

bool MediaFetcher::should_exit() {
  return !this->in_use;
}

bool MediaFetcher::is_playing() {
  if (this->media_type == MediaType::IMAGE)
    throw std::runtime_error(fmt::format("[{}] Cannot check playing state of "
    "Image media file", FUNCDINFO));
  return this->clock.is_playing();
}

void MediaFetcher::pause(double current_system_time) {
  if (this->media_type == MediaType::IMAGE)
    throw std::runtime_error(fmt::format("[{}] Cannot pause image media file",
    FUNCDINFO));
  this->clock.stop(current_system_time);
}

void MediaFetcher::resume(double current_system_time) {
  if (this->media_type == MediaType::IMAGE)
    throw std::runtime_error(fmt::format("[{}] Cannot resume image media file",
    FUNCDINFO));
  this->clock.resume(current_system_time);
  std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
  this->resume_cond.notify_all();
}

bool MediaFetcher::has_error() const noexcept {
  return this->error.has_value();
}

std::string MediaFetcher::get_error() {
  if (!this->error.has_value())
    throw std::runtime_error(fmt::format("[{}] Cannot get error when no error "
    "is available", FUNCDINFO));
  return *this->error;
}

/**
 * Thread-Safe
*/
bool MediaFetcher::has_media_stream(enum AVMediaType media_type) const {
  return this->mdec->has_stream_decoder(media_type);
}

/**
 * Thread-Safe
*/
double MediaFetcher::get_duration() const {
  return this->mdec->get_duration();
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
    throw std::runtime_error(fmt::format("[{}] Could not jump to time {} ({} "
    "seconds). Time is out of the bounds of duration {} ( {} seconds )",
    FUNCDINFO, format_time_hh_mm_ss(target_time), target_time,
    format_time_hh_mm_ss(this->get_duration()), this->get_duration()));
  }

  const double original_time = this->get_time(current_system_time);
  int ret = this->mdec->jump_to_time(target_time);

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
  
  if (this->media_type != MediaType::IMAGE && this->is_playing())
    this->pause(current_system_time);
  if (this->video_thread.joinable())
    this->video_thread.join();
  if (this->duration_checking_thread.joinable())
    this->duration_checking_thread.join();
  if (this->audio_thread.joinable())
    this->audio_thread.join();
}

