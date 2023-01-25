#include <playback.h>
#include <wtime.h>
#include <wmath.h>

#include <algorithm>

const double Playback::MAX_VOLUME;
const double Playback::MAX_SPEED;
const double Playback::MIN_VOLUME;
const double Playback::MIN_SPEED;

Playback::Playback() {
    this->m_start_time = 0.0;
    this->m_paused_time = 0.0;
    this->m_skipped_time = 0.0;
    this->m_speed = 1.0;
    this->m_volume = 1.0;
    this->m_playing = false;
};

void Playback::toggle(double current_system_time) {
    if (this->m_playing) {
        this->stop(current_system_time);
    } else {
        this->resume(current_system_time);
    }
};

void Playback::skip(double seconds_to_skip) {
    this->m_skipped_time += seconds_to_skip;
};

void Playback::start(double current_system_time) {
    this->m_playing = true;
    this->m_start_time = current_system_time;
};

void Playback::resume(double current_system_time) {
    this->m_playing = true;
    this->m_paused_time += current_system_time - this->m_last_pause_time;
};


void Playback::stop(double current_system_time) {
    this->m_playing = false;
    this->m_last_pause_time = current_system_time;
};

bool Playback::is_playing() {
    return this->m_playing;
};

void Playback::set_playing(bool playing) {
    this->m_playing = playing;
};

double Playback::get_speed() {
    return this->m_speed;
};

double Playback::get_volume() {
    return this->m_volume;
};

void Playback::set_speed(double amount) {
    this->m_speed = clamp(amount, Playback::MIN_SPEED, Playback::MAX_SPEED);
}

void Playback::set_volume(double amount) {
    this->m_volume = clamp(amount, Playback::MIN_VOLUME, Playback::MAX_VOLUME);
};

void Playback::change_speed(double offset) {
    this->set_speed(this->m_speed + offset);
};

void Playback::change_volume(double offset) {
    this->set_volume(this->m_volume + offset);
};

double Playback::get_time(double current_system_time) {
    return current_system_time - this->m_start_time - this->m_paused_time + this->m_skipped_time;
};

// double get_playback_current_time(Playback* playback) {
//     return system_clock_sec() - playback->start_time - playback->paused_time + playback->skipped_time; 
// }
