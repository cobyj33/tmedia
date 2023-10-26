#include "audiobuffer.h"

#include "wmath.h"
#include "audio.h"

#include <vector>
#include <stdexcept>


AudioBuffer::AudioBuffer(unsigned int nb_channels, unsigned int sample_rate) {
  this->m_playhead_frame = 0;
  this->m_start_time = 0.0;
  this->m_nb_channels = nb_channels;
  this->m_sample_rate = sample_rate;
};

std::size_t AudioBuffer::get_nb_frames() const {
  if (this->m_nb_channels == 0) {
    return 0;
  }
  
  return this->m_buffer.size() / (std::size_t)this->m_nb_channels;
};

double AudioBuffer::get_elapsed_time() const {
  if (this->m_sample_rate == 0) {
    return 0.0;
  }

  return (double)this->m_playhead_frame / (double)this->m_sample_rate;
};

double AudioBuffer::get_time() const {
  return this->m_start_time + this->get_elapsed_time();
};

void AudioBuffer::set_time_in_bounds(double time) {
  if (this->get_nb_frames() == 0) {
    throw std::runtime_error("[AudioBuffer::set_time_in_bounds] Cannot set audio buffer time to " + std::to_string(time) + ". The audio buffer is currently empty.");
  }
  if (!this->is_time_in_bounds(time)) {
    throw std::runtime_error("[AudioBuffer::set_time_in_bounds] Cannot set audio buffer time to " + std::to_string(time) + ". The audio buffer is currently only holding data between the times " + std::to_string(this->m_start_time) + " and " + std::to_string(this->get_end_time()));
  }

  this->m_playhead_frame = static_cast<std::size_t>((time - this->m_start_time) * this->m_sample_rate);
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
  std::size_t offset_frame_from_start = (target_time - this->m_start_time) * this->m_sample_rate;
  this->m_buffer = std::move(std::vector<float>(this->m_buffer.begin() + offset_frame_from_start * this->m_nb_channels, this->m_buffer.end()));
  this->m_start_time = target_time;
  this->m_playhead_frame -= offset_frame_from_start;
}

double AudioBuffer::get_end_time() const {
  return this->m_start_time + ((double)this->get_nb_frames() / this->m_sample_rate);
};

void AudioBuffer::write(float* frames, int nb_frames) {
  for (int i = 0; i < nb_frames * this->get_nb_channels(); i++) {
    this->m_buffer.push_back(frames[i]);
  }
};

bool AudioBuffer::is_time_in_bounds(double time) const {
  return time >= this->m_start_time && time <= this->get_end_time();
};

void AudioBuffer::clear_and_restart_at(double time) {
  this->m_buffer.clear();
  this->m_playhead_frame = 0;
  this->m_start_time = time;
};


bool AudioBuffer::can_read(std::size_t nb_frames) const {
  return this->m_playhead_frame + nb_frames <= this->get_nb_frames();
};

bool AudioBuffer::can_read() const {
  return this->can_read(1);
};

void AudioBuffer::peek_into(std::size_t nb_frames, float* target) const {
  for (std::size_t i = 0; i < nb_frames * this->m_nb_channels; i++) {
    target[i] = this->m_buffer[this->m_playhead_frame * this->m_nb_channels + i];
  }
};

std::vector<float> AudioBuffer::peek_into(std::size_t nb_frames) {
  std::vector<float> peek;
  for (std::size_t i = 0; i < nb_frames * this->m_nb_channels; i++) {
    peek.push_back(this->m_buffer[this->m_playhead_frame * this->m_nb_channels + i]);
  }

  return peek;
}


void AudioBuffer::read_into(std::size_t nb_frames, float* target)  {
  this->peek_into(nb_frames, target);
  this->advance(nb_frames);
};

void AudioBuffer::advance(std::size_t nb_frames) {
  this->m_playhead_frame += nb_frames;
};

int AudioBuffer::get_nb_channels() const {
  return this->m_nb_channels;
};

int AudioBuffer::get_sample_rate() const {
  return this->m_sample_rate;
};

std::size_t AudioBuffer::get_nb_can_read() const {
  return this->get_nb_frames() - this->m_playhead_frame;
}
