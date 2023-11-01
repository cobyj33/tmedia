#include "blocking_audioringbuffer.h"

#include <mutex>
#include <condition_variable>
#include <memory>
#include <stdexcept>
#include <vector>
#include <utility>
#include <chrono>

BlockingAudioRingBuffer::BlockingAudioRingBuffer(int size, int nb_channels) {
  this->m_ring_buffer = std::move(std::make_unique<AudioRingBuffer>(size, nb_channels));
}

int BlockingAudioRingBuffer::get_nb_channels() {
  return this->m_ring_buffer->get_nb_channels();
}

void BlockingAudioRingBuffer::clear() {
  std::lock_guard<std::mutex> lock(this->mutex);
  this->m_ring_buffer->clear();
  this->cond.notify_one();
}

void BlockingAudioRingBuffer::read_into(int nb_frames, float* out) {
  std::unique_lock<std::mutex> lock(this->mutex);
  while (this->m_ring_buffer->get_nb_can_read() < nb_frames) {
    this->cond.wait(lock);
  }

  this->m_ring_buffer->read_into(nb_frames, out);
  this->cond.notify_one();
}

bool BlockingAudioRingBuffer::try_read_into(int nb_frames, float* out, int milliseconds) {
  std::unique_lock<std::mutex> lock(this->mutex);
  if (this->m_ring_buffer->get_nb_can_read() < nb_frames) {
    this->cond.wait_for(lock, std::chrono::milliseconds(milliseconds));
  }

  if (this->m_ring_buffer->get_nb_can_read() >= nb_frames) {
    this->m_ring_buffer->read_into(nb_frames, out);
    this->cond.notify_one();
    return true;
  } else {
    this->cond.notify_one();
    return false;
  }
}


void BlockingAudioRingBuffer::peek_into(int nb_frames, float* out) {
  std::unique_lock<std::mutex> lock(this->mutex);
  while (this->m_ring_buffer->get_nb_can_read() < nb_frames) {
    this->cond.wait(lock);
  }

  this->m_ring_buffer->peek_into(nb_frames, out);
  this->cond.notify_one();
}

std::vector<float> BlockingAudioRingBuffer::peek_into(int nb_frames) {
  std::unique_lock<std::mutex> lock(this->mutex);
  while (this->m_ring_buffer->get_nb_can_read() < nb_frames) {
    this->cond.wait(lock);
  }

  std::vector<float> peek = this->m_ring_buffer->peek_into(nb_frames);
  this->cond.notify_one();
  return peek;
}

std::vector<float> BlockingAudioRingBuffer::try_peek_into(int nb_frames, int milliseconds) {
  std::unique_lock<std::mutex> lock(this->mutex);
  if (this->m_ring_buffer->get_nb_can_read() < nb_frames) {
    this->cond.wait_for(lock, std::chrono::milliseconds(milliseconds));
  }

  if (this->m_ring_buffer->get_nb_can_read() >= nb_frames) {
    std::vector<float> peek = this->m_ring_buffer->peek_into(nb_frames);
    this->cond.notify_one();
    return peek;
  } else {
    this->cond.notify_one();
    return {};
  }
}

void BlockingAudioRingBuffer::write_into(int nb_frames, float* in) {
  std::unique_lock<std::mutex> lock(this->mutex);
  while (this->m_ring_buffer->get_nb_can_write() < nb_frames) {
    this->cond.wait(lock);
  }

  this->m_ring_buffer->write_into(nb_frames, in);
  this->cond.notify_one();
}

bool BlockingAudioRingBuffer::try_write_into(int nb_frames, float* in, int milliseconds) {
  std::unique_lock<std::mutex> lock(this->mutex);
  if (this->m_ring_buffer->get_nb_can_write() < nb_frames) {
    this->cond.wait_for(lock, std::chrono::milliseconds(milliseconds));
  }

  if (this->m_ring_buffer->get_nb_can_write() >= nb_frames) {
    this->m_ring_buffer->write_into(nb_frames, in);
    this->cond.notify_one();
    return true;
  } else {
    this->cond.notify_one();
    return false;
  }
}