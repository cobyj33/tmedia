#include <media.h>
#include <wtime.h>

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

void Playback::toggle() {
    if (this->m_playing) {
        this->stop();
    } else {
        this->resume();
    }
};

void Playback::skip(double amount) {
    this->m_skipped_time += amount;
};

void Playback::start() {
    this->m_playing = true;
    this->m_start_time = clock_sec();
};

void Playback::resume() {
    this->m_playing = true;
    this->m_paused_time += clock_sec() - this->m_last_pause_time;
};


void Playback::stop() {
    this->m_playing = false;
    this->m_last_pause_time = clock_sec();
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
    this->m_speed = std::max(Playback::MIN_SPEED, std::min(Playback::MAX_SPEED, amount));
};

void Playback::set_volume(double amount) {
    this->m_volume = std::max(Playback::MIN_VOLUME, std::min(Playback::MAX_VOLUME, amount));
};

void Playback::change_speed(double offset) {
    this->set_speed(this->m_speed + offset);
};

void Playback::change_volume(double offset) {
    this->set_volume(this->m_volume + offset);
};



double Playback::get_time() {
    return clock_sec() - this->m_start_time - this->m_paused_time + this->m_skipped_time;
};

// double get_playback_current_time(Playback* playback) {
//     return clock_sec() - playback->start_time - playback->paused_time + playback->skipped_time; 
// }
