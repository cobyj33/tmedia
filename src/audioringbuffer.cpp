#include "audioringbuffer.h"

#include "formatting.h"
#include "funcmac.h"

#include <fmt/format.h>

#include <vector>
#include <stdexcept>
#include <system_error>
#include <cstddef>

#include <cassert>

/**
 * Implementation details:
 * 
 * m_head and m_tail are INDEXES into the internal AudioRingBuffer vector.
*/

AudioRingBuffer::AudioRingBuffer(int frame_capacity, int nb_channels, int sample_rate, double playback_start_time) {
  this->m_nb_channels = nb_channels;
  this->m_head = 0;
  this->m_tail = 1;
  this->m_size_frames = frame_capacity;
  this->m_size_samples = frame_capacity * nb_channels;
  this->rb.reserve(this->m_size_samples);

  this->m_start_time = playback_start_time;
  this->m_sample_rate = sample_rate;
  this->m_frames_read = 0;
}


void AudioRingBuffer::clear(double new_start_time) {
  this->m_head = 0;
  this->m_tail = 1;
  this->m_frames_read = 0;
  this->m_start_time = new_start_time;
}

double AudioRingBuffer::get_buffer_end_time() {
  return this->m_start_time + ((double)(this->m_frames_read + (std::size_t)this->get_frames_can_read()) / (double)this->m_sample_rate);
}

double AudioRingBuffer::get_buffer_current_time() {
  return this->m_start_time + ((double)this->m_frames_read / (double)this->m_sample_rate);
}

bool AudioRingBuffer::is_time_in_bounds(double playback_time) {
  if (playback_time < this->get_buffer_current_time()) {
    return false;
  } else if (playback_time > this->get_buffer_end_time()) {
    return false;
  }
  return true;
}

void AudioRingBuffer::set_time_in_bounds(double playback_time) {
  assert(is_time_in_bounds(playback_time));

  const double time_offset = playback_time - this->get_buffer_current_time();
  const int frame_offset = this->m_sample_rate * time_offset;
  this->m_head = (this->m_head + frame_offset) % this->m_size_samples;
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
  assert(this->get_frames_can_read() >= nb_frames);

  for (int i = 0; i < nb_frames * this->m_nb_channels; i++) {
    out[i] = this->rb[this->m_head];
    this->m_head++;
    if (this->m_head >= this->m_size_samples) this->m_head = 0;
  }
  this->m_frames_read += nb_frames;
}

void AudioRingBuffer::peek_into(int nb_frames, float* out) {
  assert(this->get_frames_can_read() >= nb_frames);

  int original_head = this->m_head;
  this->read_into(nb_frames, out);
  this->m_head = original_head;
  this->m_frames_read -= nb_frames;
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
  assert(this->get_frames_can_write() >= nb_frames);

  for (int i = 0; i < nb_frames * this->m_nb_channels; i++) {
    this->rb[this->m_tail] = in[i];
    this->m_tail++;
    if (this->m_tail >= this->m_size_samples) this->m_tail = 0;
  }
}

