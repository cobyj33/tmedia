#include "mediaclock.h"

#include "funcmac.h"
#include <fmt/format.h>

#include <stdexcept>

MediaClock::MediaClock() {
  this->m_start_time = 0.0;
  this->m_paused_time = 0.0;
  this->m_skipped_time = 0.0;
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

void MediaClock::init(double current_system_time) {
  if (this->is_playing()) {
    throw std::runtime_error(fmt::format("[{}] Cannot start while playback is "
    "already playing.", FUNCDINFO));
  }

  this->m_playing = true;
  this->m_start_time = current_system_time;
  this->m_skipped_time = 0.0;
  this->m_paused_time = 0.0;
};

void MediaClock::resume(double current_system_time) {
  if (this->is_playing()) {
    throw std::runtime_error(fmt::format("[{}] Cannot resume while playback "
    "is already playing", FUNCDINFO));
  }

  this->m_playing = true;
  this->m_paused_time += current_system_time - this->m_last_pause_system_time;
};


void MediaClock::stop(double current_system_time) {
  if (!this->is_playing()) {
    throw std::runtime_error(fmt::format("[{}] Attempted to stop playback "
    "although playback is already stopped", FUNCDINFO));
  }

  this->m_playing = false;
  this->m_last_pause_system_time = current_system_time;
};

bool MediaClock::is_playing() const {
  return this->m_playing;
};


double MediaClock::get_time(double current_system_time) const {
  switch (this->m_playing) {
    case true: return current_system_time - this->m_start_time - this->m_paused_time + this->m_skipped_time;
    case false: return this->m_last_pause_system_time - this->m_start_time - this->m_paused_time + this->m_skipped_time;
  }
};

