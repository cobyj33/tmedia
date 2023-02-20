#include <algorithm>
#include <stdexcept>

#include <playback.h>
#include <wtime.h>
#include <wmath.h>



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
    if (this->is_playing()) {
        throw std::runtime_error("Cannot resume while playback is already playing");
    }

    this->m_playing = true;
    this->m_start_time = current_system_time;

    this->m_skipped_time = 0.0;
    this->m_paused_time = 0.0;
};

void Playback::resume(double current_system_time) {
    if (this->is_playing()) {
        throw std::runtime_error("Cannot resume while playback is already playing");
    }

    this->m_playing = true;
    this->m_paused_time += current_system_time - this->m_last_pause_time;
};


void Playback::stop(double current_system_time) {
    if (!this->is_playing()) {
        throw std::runtime_error("Attempted to stop playback although playback is already stopped");
    }

    this->m_playing = false;
    this->m_last_pause_time = current_system_time;
};

bool Playback::is_playing() const {
    return this->m_playing;
};


double Playback::get_speed() const {
    return this->m_speed;
};

double Playback::get_volume() const {
    return this->m_volume;
};

void Playback::set_speed(double amount) {
    this->m_speed = amount;
}

void Playback::set_volume(double amount) {
    this->m_volume = amount;
};

void Playback::change_speed(double offset) {
    this->set_speed(this->m_speed + offset);
};

void Playback::change_volume(double offset) {
    this->set_volume(this->m_volume + offset);
};

double Playback::get_time(double current_system_time) const {
    return current_system_time - this->m_start_time - this->m_paused_time + this->m_skipped_time;
};

