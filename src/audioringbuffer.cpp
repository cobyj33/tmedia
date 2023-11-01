#include "audioringbuffer.h"

#include <vector>
#include <stdexcept>
#include <system_error>

AudioRingBuffer::AudioRingBuffer(int size, int nb_channels, int sample_rate) {
  this->m_sample_rate = sample_rate;
  this->m_nb_channels = nb_channels;
  this->m_ring_buffer.reserve(size);
  this->m_head = 0;
  this->m_tail = 1;
  this->m_size = size;
}

int AudioRingBuffer::get_nb_channels() {
  return this->m_nb_channels;
}

int AudioRingBuffer::get_sample_rate() {
  return this->m_sample_rate;
}

void AudioRingBuffer::clear() {
  this->m_head = 0;
  this->m_tail = 1;
}

int AudioRingBuffer::get_nb_can_read() {
  if (this->m_head < this->m_tail) return (this->m_tail - this->m_head) / this->m_nb_channels;
  if (this->m_tail < this->m_head) return (this->m_size - this->m_head + this->m_tail) / this->m_nb_channels;
  return 0;
}

int AudioRingBuffer::get_nb_can_write() {
  if (this->m_head < this->m_tail) return (this->m_size - this->m_tail + this->m_head) / this->m_nb_channels;
  if (this->m_tail < this->m_head) return (this->m_head - this->m_tail) / this->m_nb_channels;
  return 0;
}

void AudioRingBuffer::read_into(int nb_frames, float* out) {
  if (this->get_nb_can_read() < nb_frames) {
    throw std::runtime_error("[AudioRingBuffer::read_into] Cannot read " + 
    std::to_string(nb_frames) + " frames ( size: " + std::to_string(this->m_size) + " can read: " + std::to_string(this->get_nb_can_read()) + ")"); 
  }

  for (int i = 0; i < nb_frames * this->m_nb_channels; i++) {
    out[i] = this->m_ring_buffer[this->m_head];
    this->m_head++;
    if (this->m_head >= this->m_size) this->m_head = 0;
  }
}

void AudioRingBuffer::peek_into(int nb_frames, float* out) {
  if (this->get_nb_can_read() < nb_frames) {
    throw std::runtime_error("[AudioRingBuffer::peek_into] Cannot read " + 
    std::to_string(nb_frames) + " frames ( size: " + std::to_string(this->m_size) + " can read: " + std::to_string(this->get_nb_can_read()) + ")"); 
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
  if (this->get_nb_can_write() < nb_frames) {
    throw std::runtime_error("[AudioRingBuffer::write_into] Cannot write " + 
    std::to_string(nb_frames) + " frames ( size: " + std::to_string(this->m_size) + " can write: " + std::to_string(this->get_nb_can_write()) + ")"); 
  }

  for (int i = 0; i < nb_frames * this->m_nb_channels; i++) {
    this->m_ring_buffer[this->m_tail] = in[i];
    this->m_tail++;
    if (this->m_tail >= this->m_size) this->m_tail = 0;
  }
}

