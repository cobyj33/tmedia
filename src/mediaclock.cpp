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

void MediaClock::toggle(double currsystime) {
  if (this->m_playing) {
    this->stop(currsystime);
  } else {
    this->resume(currsystime);
  }
};

void MediaClock::skip(double seconds_to_skip) {
  this->m_skipped_time += seconds_to_skip;
};

void MediaClock::init(double currsystime) {
  if (this->is_playing()) {
    throw std::runtime_error(fmt::format("[{}] Cannot start while playback is "
    "already playing.", FUNCDINFO));
  }

  this->m_playing = true;
  this->m_start_time = currsystime;
  this->m_skipped_time = 0.0;
  this->m_paused_time = 0.0;
};

void MediaClock::resume(double currsystime) {
  if (this->is_playing()) return;

  this->m_playing = true;
  this->m_paused_time += currsystime - this->m_last_pause_system_time;
};


void MediaClock::stop(double currsystime) {
  if (!this->is_playing()) return;

  this->m_playing = false;
  this->m_last_pause_system_time = currsystime;
};

bool MediaClock::is_playing() const {
  return this->m_playing;
};


double MediaClock::get_time(double currsystime) const {
  switch (this->m_playing) {
    case true: return currsystime - this->m_start_time - this->m_paused_time + this->m_skipped_time;
    case false: return this->m_last_pause_system_time - this->m_start_time - this->m_paused_time + this->m_skipped_time;
  }
};

