#include "blocking_audioringbuffer.h"

#include <mutex>
#include <condition_variable>
#include <memory>
#include <stdexcept>
#include <vector>
#include <utility>
#include <chrono>

BlockingAudioRingBuffer::BlockingAudioRingBuffer(int frame_capacity, int nb_channels, int sample_rate, double playback_start_time) {
  this->rb = std::move(std::make_unique<AudioRingBuffer>(frame_capacity, nb_channels, sample_rate, playback_start_time));
}


double BlockingAudioRingBuffer::get_buffer_current_time() {
  std::unique_lock<std::mutex> lock(this->mutex);
  return this->rb->get_buffer_current_time();
}

bool BlockingAudioRingBuffer::is_time_in_bounds(double playback_time) {
  std::unique_lock<std::mutex> lock(this->mutex);
  return this->rb->is_time_in_bounds(playback_time);
}

bool BlockingAudioRingBuffer::try_set_time_in_bounds(double playback_time, int milliseconds) {
  std::unique_lock<std::mutex> lock(this->mutex);
  if (!this->rb->is_time_in_bounds(playback_time)) {
    this->cond.wait_for(lock, std::chrono::milliseconds(milliseconds));
  }

  if (this->rb->is_time_in_bounds(playback_time)) {
    this->rb->set_time_in_bounds(playback_time);
    this->cond.notify_one();
    return true;
  } else {
    this->cond.notify_one();
    return false;
  }
}

void BlockingAudioRingBuffer::clear(double new_start_time) {
  std::lock_guard<std::mutex> lock(this->mutex);
  this->rb->clear(new_start_time);
  this->cond.notify_one();
}

void BlockingAudioRingBuffer::read_into(int nb_frames, float* out) {
  std::unique_lock<std::mutex> lock(this->mutex);
  while (this->rb->get_frames_can_read() < nb_frames) {
    this->cond.wait(lock);
  }

  this->rb->read_into(nb_frames, out);
  this->cond.notify_one();
}

bool BlockingAudioRingBuffer::try_read_into(int nb_frames, float* out, int milliseconds) {
  std::unique_lock<std::mutex> lock(this->mutex);
  if (this->rb->get_frames_can_read() < nb_frames) {
    this->cond.wait_for(lock, std::chrono::milliseconds(milliseconds));
  }

  if (this->rb->get_frames_can_read() >= nb_frames) {
    this->rb->read_into(nb_frames, out);
    this->cond.notify_one();
    return true;
  } else {
    this->cond.notify_one();
    return false;
  }
}


void BlockingAudioRingBuffer::peek_into(int nb_frames, float* out) {
  std::unique_lock<std::mutex> lock(this->mutex);
  while (this->rb->get_frames_can_read() < nb_frames) {
    this->cond.wait(lock);
  }

  this->rb->peek_into(nb_frames, out);
  this->cond.notify_one();
}

bool BlockingAudioRingBuffer::try_peek_into(int nb_frames, float* out, int milliseconds) {
  std::unique_lock<std::mutex> lock(this->mutex);
  if (this->rb->get_frames_can_read() < nb_frames) {
    this->cond.wait_for(lock, std::chrono::milliseconds(milliseconds));
  }

  if (this->rb->get_frames_can_read() >= nb_frames) {
    this->rb->peek_into(nb_frames, out);
    this->cond.notify_one();
    return true;
  }
  this->cond.notify_one();
  return false;
}

void BlockingAudioRingBuffer::write_into(int nb_frames, float* in) {
  std::unique_lock<std::mutex> lock(this->mutex);
  while (this->rb->get_frames_can_write() < nb_frames) {
    this->cond.wait(lock);
  }

  this->rb->write_into(nb_frames, in);
  this->cond.notify_one();
}

bool BlockingAudioRingBuffer::try_write_into(int nb_frames, float* in, int milliseconds) {
  std::unique_lock<std::mutex> lock(this->mutex);
  if (this->rb->get_frames_can_write() < nb_frames) {
    this->cond.wait_for(lock, std::chrono::milliseconds(milliseconds));
  }

  if (this->rb->get_frames_can_write() >= nb_frames) {
    this->rb->write_into(nb_frames, in);
    this->cond.notify_one();
    return true;
  } else {
    this->cond.notify_one();
    return false;
  }
}