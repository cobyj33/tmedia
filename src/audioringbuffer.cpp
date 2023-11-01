#include "audioringbuffer.h"

#include <vector>
#include <stdexcept>
#include <system_error>

/**
 * Implementation details:
 * 
 * m_head and m_tail are INDEXES into the internal AudioRingBuffer vector.
*/

AudioRingBuffer::AudioRingBuffer(int frame_capacity, int nb_channels) {
  this->m_nb_channels = nb_channels;
  this->m_head = 0;
  this->m_tail = 1;
  this->m_size_frames = frame_capacity;
  this->m_size_samples = frame_capacity * nb_channels;
  this->m_ring_buffer.reserve(this->m_size_samples);
}

int AudioRingBuffer::get_nb_channels() {
  return this->m_nb_channels;
}

void AudioRingBuffer::clear() {
  this->m_head = 0;
  this->m_tail = 1;
}

int AudioRingBuffer::get_frames_can_read() {
  if (this->m_head < this->m_tail) return (this->m_tail - this->m_head) / this->m_nb_channels;
  if (this->m_tail < this->m_head) return (this->m_size_samples - this->m_head + this->m_tail) / this->m_nb_channels;
  return 0;
}

int AudioRingBuffer::get_frames_can_write() {
  if (this->m_head < this->m_tail) return (this->m_size_samples - this->m_tail + this->m_head) / this->m_nb_channels;
  if (this->m_tail < this->m_head) return (this->m_head - this->m_tail) / this->m_nb_channels;
  return 0;
}

void AudioRingBuffer::read_into(int nb_frames, float* out) {
  if (this->get_frames_can_read() < nb_frames) {
    throw std::runtime_error("[AudioRingBuffer::read_into] Cannot read " + 
    std::to_string(nb_frames) + " frames ( size: " + std::to_string(this->m_size_frames) + " can read: " + std::to_string(this->get_frames_can_read()) + ")"); 
  }

  for (int i = 0; i < nb_frames * this->m_nb_channels; i++) {
    out[i] = this->m_ring_buffer[this->m_head];
    this->m_head++;
    if (this->m_head >= this->m_size_samples) this->m_head = 0;
  }
}

void AudioRingBuffer::peek_into(int nb_frames, float* out) {
  if (this->get_frames_can_read() < nb_frames) {
    throw std::runtime_error("[AudioRingBuffer::peek_into] Cannot read " + 
    std::to_string(nb_frames) + " frames ( size: " + std::to_string(this->m_size_frames) + " can read: " + std::to_string(this->get_frames_can_read()) + ")"); 
  }

  int original_head = this->m_head;
  this->read_into(nb_frames, out);
  this->m_head = original_head;
}

std::vector<float> AudioRingBuffer::peek_into(int nb_frames) {
  float* temp = new float[nb_frames * this->m_nb_channels];
  this->peek_into(nb_frames, temp);
  std::vector<float> res;
  for (int i = 0; i < nb_frames * this->m_nb_channels; i++) {
    res.push_back(temp[i]);
  }
  delete[] temp;
  return res;
}

void AudioRingBuffer::write_into(int nb_frames, float* in) {
  if (this->get_frames_can_write() < nb_frames) {
    throw std::runtime_error("[AudioRingBuffer::write_into] Cannot write " + 
    std::to_string(nb_frames) + " frames ( size: " + std::to_string(this->m_size_frames) + " can write: " + std::to_string(this->get_frames_can_write()) + ")"); 
  }

  for (int i = 0; i < nb_frames * this->m_nb_channels; i++) {
    this->m_ring_buffer[this->m_tail] = in[i];
    this->m_tail++;
    if (this->m_tail >= this->m_size_samples) this->m_tail = 0;
  }
}

