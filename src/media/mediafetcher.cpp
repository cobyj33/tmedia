
#include <tmedia/media/mediafetcher.h>

#include <tmedia/ffmpeg/decode.h>
#include <tmedia/util/wtime.h>
#include <tmedia/util/wmath.h>
#include <tmedia/util/formatting.h>
#include <tmedia/ffmpeg/audioresampler.h>
#include <tmedia/util/defines.h>

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <set>

#include <fmt/format.h>

#include <cassert>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

MediaFetcher::MediaFetcher(const std::filesystem::path& path, const std::array<bool, AVMEDIA_TYPE_NB>& requested_streams) :
  path(path), mdec(std::make_unique<MediaDecoder>(path, requested_streams)) {
  this->in_use = false;

  if (this->mdec->nb_stream_decoders() == 0)
    throw std::runtime_error(fmt::format("[{}] Could not find any media streams", FUNCDINFO));

  this->media_type = this->mdec->get_media_type();
  this->msg_video_jump_curr_time = 0;
  this->msg_audio_jump_curr_time = 0;
  this->frame_changed = false;
  pixdata_setnewdims(this->frame, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);

  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    static constexpr int MINIMUM_INTERNAL_AUDIO_BUFFER_SIZE_FRAMES = 4096;
    static constexpr double INTERNAL_AUDIO_BUFFER_LENGTH_SECONDS = 0.1;
    const int sample_rate = this->mdec->get_sample_rate();
    const int nb_channels = this->mdec->get_nb_channels();
    const int requested_frame_capacity = sample_rate * nb_channels * INTERNAL_AUDIO_BUFFER_LENGTH_SECONDS;
    const int frame_capacity = std::max(MINIMUM_INTERNAL_AUDIO_BUFFER_SIZE_FRAMES, requested_frame_capacity);
    this->audio_buffer = std::make_unique<BlockingAudioRingBuffer>(frame_capacity, nb_channels, sample_rate, 0.0);
  }
}

void MediaFetcher::dispatch_exit(std::string_view err) {
  this->error = std::string(err);
  this->dispatch_exit();
}

void MediaFetcher::dispatch_exit() {
  std::scoped_lock<std::mutex, std::mutex> notification_locks(this->ex_noti_mtx, this->resume_notify_mutex);
  this->in_use = false;
  this->exit_cond.notify_all();
  this->resume_cond.notify_all();
}

bool MediaFetcher::is_playing() {
  return this->clock.is_playing();
}

void MediaFetcher::pause(double currsystime) {
  if (this->media_type == MediaType::IMAGE)
    throw std::runtime_error(fmt::format("[{}] Cannot pause image media file",
    FUNCDINFO));
  this->clock.stop(currsystime);
}

void MediaFetcher::resume(double currsystime) {
  if (this->media_type == MediaType::IMAGE)
    throw std::runtime_error(fmt::format("[{}] Cannot resume image media file",
    FUNCDINFO));
  this->clock.resume(currsystime);
  std::unique_lock<std::mutex> resume_notify_lock(this->resume_notify_mutex);
  this->resume_cond.notify_all();
}


/**
 * For threadsafety, alter_mutex must be locked
*/
double MediaFetcher::get_audio_desync_time(double currsystime) const {
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    double playback_time = this->get_time(currsystime);
    double audio_time = this->audio_buffer->get_buffer_current_time();
    return std::abs(audio_time - playback_time);
  }
  // Video itself doesn't really get desynced since the video thread syncs
  // itself to the MediaClock (see video_thread.cpp)
  return 0.0; 
}

/**
 * alter_mutex must be locked
*/
int MediaFetcher::jump_to_time(double target_time, double currsystime) {
  assert(target_time >= 0.0 && target_time <= this->get_duration());
  const double original_time = this->get_time(currsystime);
  this->msg_audio_jump_curr_time++;
  this->msg_video_jump_curr_time++;
  
  this->clock.skip(target_time - original_time); // Update the playback to account for the skipped time
  return 0; // assume success
}

void MediaFetcher::begin(double currsystime) {
  this->in_use = true;
  this->clock.init(currsystime);
  
  std::thread ivt(&MediaFetcher::video_fetching_thread_func, this);
  this->video_thread.swap(ivt);
  
  if (this->media_type == MediaType::VIDEO || this->media_type == MediaType::AUDIO) {
    std::thread idct(&MediaFetcher::duration_checking_thread_func, this);
    this->duration_checking_thread.swap(idct);
  }
  
  if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
    std::thread iat(&MediaFetcher::audio_dispatch_thread_func, this);
    this->audio_thread.swap(iat);
  }
}

void MediaFetcher::join(double currsystime) {
  this->in_use = false; // the user can set this as well if they want, but this is to make sure that the threads WILL end regardless
  
  if (this->media_type != MediaType::IMAGE && this->is_playing())
    this->pause(currsystime);
  if (this->video_thread.joinable())
    this->video_thread.join();
  if (this->duration_checking_thread.joinable())
    this->duration_checking_thread.join();
  if (this->audio_thread.joinable())
    this->audio_thread.join();
}

