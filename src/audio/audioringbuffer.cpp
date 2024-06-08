#include <tmedia/audio/audioringbuffer.h>

#include <cassert>
/**
 * Implementation details:
 * 
 * m_head and m_tail are INDEXES into the internal AudioRingBuffer vector. Since
 * AudioRingBuffer is a circular buffer, m_head is not always less than m_tail.
 * 
 * I wrote that AudioRingBuffer exhibits "undefined behavior" in the header
 * whenever we read or peak any frame count greater than get_frames_can_read
 * or write any frame count greater than get_frames_can_write, but it'll
 * actually just overwrite itself on the first call. After that, it'll
 * just get confused on how big the buffer really is.
*/

AudioRingBuffer::AudioRingBuffer(int frame_capacity, int nb_channels, int sample_rate, double playback_start_time) {
  assert(frame_capacity >= 0);
  assert(nb_channels > 0);
  assert(sample_rate > 0);
  assert(playback_start_time >= 0.0);

  this->m_nb_channels = nb_channels;
  this->m_head = 0;
  this->m_tail = 1;
  this->m_size_frames = frame_capacity;
  this->m_size_samples = frame_capacity * nb_channels;
  this->rb = new float[frame_capacity * nb_channels];

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
  return this->m_start_time + (static_cast<double>(this->m_frames_read + static_cast<unsigned long long>(this->get_frames_can_read())) / static_cast<double>(this->m_sample_rate));
}

double AudioRingBuffer::get_buffer_current_time() {
  return this->m_start_time + (static_cast<double>(this->m_frames_read) / static_cast<double>(this->m_sample_rate));
}

bool AudioRingBuffer::is_time_in_bounds(double playback_time) {
  return playback_time >= this->get_buffer_current_time() && playback_time <= this->get_buffer_end_time();
}

void AudioRingBuffer::set_time_in_bounds(double playback_time) {
  assert(is_time_in_bounds(playback_time));

  const double time_offset = playback_time - this->get_buffer_current_time();
  const int frame_offset = this->m_sample_rate * time_offset;
  this->m_head = (this->m_head + frame_offset) % this->m_size_samples;
}

int AudioRingBuffer::get_frames_can_read() {
  // Can you tell I just learned about branchless programming :o
  // I don't even know if it's actually 'faster'

  return ((this->m_head < this->m_tail) * (this->m_tail - this->m_head) +
    (this->m_tail < this->m_head) * (this->m_size_samples - this->m_head + this->m_tail)) / this->m_nb_channels;
}

int AudioRingBuffer::get_frames_can_write() {
  return ((this->m_head < this->m_tail) * (this->m_size_samples - this->m_tail + this->m_head) +
    (this->m_tail < this->m_head) * (this->m_head - this->m_tail)) / this->m_nb_channels;
}

void AudioRingBuffer::read_into(int nb_frames, float* out) {
  assert(this->get_frames_can_read() >= nb_frames);
  const int nb_samples = nb_frames * this->m_nb_channels;

  for (int i = 0; i < nb_samples; i++) {
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

void AudioRingBuffer::write_into(int nb_frames, float* in) {
  assert(this->get_frames_can_write() >= nb_frames);

  const int nb_samples = nb_frames * this->m_nb_channels;
  for (int i = 0; i < nb_samples; i++) {
    this->rb[this->m_tail] = in[i];
    this->m_tail++;
    if (this->m_tail >= this->m_size_samples) this->m_tail = 0;
  }
}

AudioRingBuffer::~AudioRingBuffer() {
  delete[] this->rb;
}
