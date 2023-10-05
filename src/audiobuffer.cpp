#include <cstdint>
#include <stdexcept>

#include "audiobuffer.h"
#include "wmath.h"
#include "audio.h"

AudioBuffer::AudioBuffer(unsigned int nb_channels, unsigned int sample_rate) {
  this->m_playhead = 0;
  this->m_start_time = 0.0;
  this->m_nb_channels = nb_channels;
  this->m_sample_rate = sample_rate;
};

std::size_t AudioBuffer::get_nb_samples() const {
  if (this->m_nb_channels == 0) {
    return 0;
  }
  
  return (std::size_t)this->m_buffer.size() / this->m_nb_channels;
};

// NOTE: Am I supposed to also clear the start_time? 

void AudioBuffer::clear() { // Note: No longer takes in a clear capacity
  this->m_buffer.clear();
  this->m_playhead = 0;
};

double AudioBuffer::get_elapsed_time() const {
  if (this->m_sample_rate == 0) {
    return 0.0;
  }

  return (double)this->m_playhead / (double)this->m_sample_rate;
};

double AudioBuffer::get_time() const {
  return this->m_start_time + this->get_elapsed_time();
};

void AudioBuffer::set_time_in_bounds(double time) {
  if (this->get_nb_samples() == 0) {
    throw std::runtime_error("[AudioBuffer::set_time_in_bounds] Cannot set audio buffer time to " + std::to_string(time) + ". The audio buffer is currently empty.");
  }
  if (!this->is_time_in_bounds(time)) {
    throw std::runtime_error("[AudioBuffer::set_time_in_bounds] Cannot set audio buffer time to " + std::to_string(time) + ". The audio buffer is currently only holding data between the times " + std::to_string(this->m_start_time) + " and " + std::to_string(this->get_end_time()));
  }

  this->m_playhead = (std::size_t)(std::min(this->get_nb_samples() - 1, std::max( (std::size_t)0, (std::size_t)((time - this->m_start_time) * this->m_sample_rate  ))  )  );
};

double AudioBuffer::get_start_time() const {
  return this->m_start_time;
}

void AudioBuffer::leave_behind(double time) {
  double time_behind = this->get_elapsed_time();
  if (time < 0) {
    throw std::runtime_error("[AudioBuffer::leave_behind] Attempted to leave behind a negative amount of time. ( got: " + std::to_string(time) + " )");
  }
  if (time > time_behind) {
    throw std::runtime_error("[AudioBuffer::leave_behind] Attempted to leave behind more time than available in buffer. ( got: " + std::to_string(time) + " )");
  } else if (time == time_behind) {
    return;
  }

  double target_time = this->get_time() - time;
  std::size_t offset_sample_from_start = (target_time - this->m_start_time) * this->m_sample_rate;
  this->m_buffer = std::move(std::vector<uint8_t>(this->m_buffer.begin() + offset_sample_from_start * this->m_nb_channels, this->m_buffer.end()));
  this->m_start_time = target_time;
  this->m_playhead -= offset_sample_from_start;
}

double AudioBuffer::get_end_time() const {
  return this->m_start_time + ((double)this->get_nb_samples() / this->m_sample_rate);
};

void AudioBuffer::write(uint8_t sample_part) {
  this->m_buffer.push_back(sample_part);
};

void AudioBuffer::write(float sample_part) {
  this->m_buffer.push_back(normalized_float_sample_to_uint8(sample_part));
};

void AudioBuffer::write(float* samples, int nb_samples) {
  for (int i = 0; i < nb_samples * this->get_nb_channels(); i++) {
    this->write(normalized_float_sample_to_uint8(samples[i]));
  }
};

bool AudioBuffer::is_time_in_bounds(double time) const {
  return time >= this->m_start_time && time <= this->get_end_time();
};

void AudioBuffer::clear_and_restart_at(double time) {
  this->clear();
  this->m_start_time = time;
};


bool AudioBuffer::can_read(std::size_t nb_samples) const {
  return this->m_playhead + nb_samples <= this->get_nb_samples();
};

bool AudioBuffer::can_read() const {
  return this->can_read(1);
};

void AudioBuffer::peek_into(std::size_t nb_samples, float* target) const {
  for (std::size_t i = 0; i < nb_samples * this->m_nb_channels; i++) {
    target[i] = uint8_sample_to_normalized_float(this->m_buffer[this->m_playhead * this->m_nb_channels + i]);
  }
};

void AudioBuffer::read_into(std::size_t nb_samples, float* target)  {
  this->peek_into(nb_samples, target);
  this->advance(nb_samples);
};

void AudioBuffer::advance(std::size_t nb_samples) {
  this->m_playhead += nb_samples;
};

int AudioBuffer::get_nb_channels() const {
  return this->m_nb_channels;
};

int AudioBuffer::get_sample_rate() const {
  return this->m_sample_rate;
};

std::size_t AudioBuffer::get_nb_can_read() const {
  return this->get_nb_samples() - this->m_playhead;
}
