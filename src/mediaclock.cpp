#include <algorithm>
#include <stdexcept>

#include <mediaclock.h>
#include <wtime.h>
#include <wmath.h>



MediaClock::MediaClock() {
  this->m_start_time = 0.0;
  this->m_paused_time = 0.0;
  this->m_skipped_time = 0.0;
  this->m_speed = 1.0;
  this->m_volume = 1.0;
  this->m_playing = false;
};

void MediaClock::toggle(double current_system_time) {
  if (this->m_playing) {
    this->stop(current_system_time);
  } else {
    this->resume(current_system_time);
  }
};

void MediaClock::skip(double seconds_to_skip) {
  this->m_skipped_time += seconds_to_skip;
};

void MediaClock::start(double current_system_time) {
  if (this->is_playing()) {
    throw std::runtime_error("Cannot resume while playback is already playing");
  }

  this->m_playing = true;
  this->m_start_time = current_system_time;

  this->m_skipped_time = 0.0;
  this->m_paused_time = 0.0;
};

void MediaClock::resume(double current_system_time) {
  if (this->is_playing()) {
    throw std::runtime_error("Cannot resume while playback is already playing");
  }

  this->m_playing = true;
  this->m_paused_time += current_system_time - this->m_last_pause_system_time;
};


void MediaClock::stop(double current_system_time) {
  if (!this->is_playing()) {
    throw std::runtime_error("Attempted to stop playback although playback is already stopped");
  }

  this->m_playing = false;
  this->m_last_pause_system_time = current_system_time;
};

bool MediaClock::is_playing() const {
  return this->m_playing;
};


double MediaClock::get_speed() const {
  return this->m_speed;
};

double MediaClock::get_volume() const {
  return this->m_volume;
};

void MediaClock::set_speed(double amount) {
  this->m_speed = amount;
}

void MediaClock::set_volume(double amount) {
  this->m_volume = amount;
};

void MediaClock::change_speed(double offset) {
  this->set_speed(this->m_speed + offset);
};

void MediaClock::change_volume(double offset) {
  this->set_volume(this->m_volume + offset);
};

double MediaClock::get_time(double current_system_time) const {
  return current_system_time - this->m_start_time - this->m_paused_time + this->m_skipped_time;
};

