#include <cstdint>
#include <stdexcept>

#include "audiostream.h"
#include "wmath.h"
#include "audio.h"

void AudioStream::construct() {
    this->m_playhead = 0;
    this->m_nb_channels = 0;
    this->m_start_time = 0.0;
    this->m_sample_rate = 0;
    this->m_initialized = false;
}

AudioStream::AudioStream() {
    this->construct();
};

AudioStream::AudioStream(int nb_channels, int sample_rate) {
    this->construct();
    this->init(nb_channels, sample_rate);
};

std::size_t AudioStream::get_nb_samples() const {
    if (this->m_nb_channels == 0) {
        return 0;
    }
    
    return (std::size_t)this->m_stream.size() / this->m_nb_channels;
};

void AudioStream::init(int nb_channels, int sample_rate) {
    if (nb_channels <= 0) {
        std::invalid_argument("Attemped to initialize audio stream with " + std::to_string(nb_channels) + " channels. Number of channels must be an integer greater than 0");
    }

    if (sample_rate <= 0) {
        throw std::invalid_argument("Attemped to initialize audio stream with a sample rate of " + std::to_string(sample_rate) + ". Sample rate must be an integer greater than 0");
    }

    this->m_stream.clear();
    this->m_stream.shrink_to_fit();



    this->m_nb_channels = nb_channels;
    this->m_start_time = 0.0;
    this->m_sample_rate = sample_rate;
    this->m_playhead = 0;
    if (nb_channels > 0 && sample_rate > 0) {
        this->m_initialized = true;
    } else {
        if (nb_channels <= 0 && sample_rate <= 0) {
            throw std::invalid_argument("Cannot initialize audio stream, nb_channels ( " + std::to_string(nb_channels) + " ) and sample rate ( " + std::to_string(sample_rate) + "  must be positive integers ");
        } else if (nb_channels <= 0) {
            throw std::invalid_argument("Cannot initialize audio stream, nb_channels ( " + std::to_string(nb_channels) + " ) must be a positive integer ");
        } else if (sample_rate <= 0) {
            throw std::invalid_argument("Cannot initialize audio stream, sample rate ( " + std::to_string(sample_rate) + "  must be a positive integer ");
        }
    }
};

// NOTE: Am I supposed to also clear the start_time? 

void AudioStream::clear() { // Note: No longer takes in a clear capacity
    this->m_stream.clear();
    this->m_stream.shrink_to_fit();
    this->m_playhead = 0;
};

double AudioStream::get_elapsed_time() const {
    if (this->m_sample_rate == 0) {
        return 0.0;
    }

    return (double)this->m_playhead / (double)this->m_sample_rate;
};

double AudioStream::get_time() const {
    return this->m_start_time + this->get_elapsed_time();
};

std::size_t AudioStream::set_time(double time) {
    if (this->get_nb_samples() == 0) return 0.0;
    this->m_playhead = (std::size_t)(std::min(this->get_nb_samples() - 1, std::max( (std::size_t)0, (std::size_t)(time - this->m_start_time) * this->m_sample_rate  )  )  );
    return this->m_playhead;
};

double AudioStream::get_end_time() const {
    return this->m_start_time + ((double)this->get_nb_samples() / this->m_sample_rate);
};

void AudioStream::write(uint8_t sample_part) {
    this->m_stream.push_back(sample_part);
};

void AudioStream::write(float sample_part) {
    this->m_stream.push_back(normalized_float_sample_to_uint8(sample_part));
};

void AudioStream::write(float* samples, int nb_samples) {
    for (int i = 0; i < nb_samples * this->get_nb_channels(); i++) {
        this->write(normalized_float_sample_to_uint8(samples[i]));
    }
};

bool AudioStream::is_time_in_bounds(double time) const {
    return time >= this->m_start_time && time <= this->get_end_time();
};

void AudioStream::clear_and_restart_at(double time) {
    this->clear();
    this->m_start_time = time;
};


bool AudioStream::can_read(std::size_t nb_samples) const {
    return this->m_playhead + nb_samples <= this->get_nb_samples();
};

bool AudioStream::can_read() const {
    return this->can_read(1);
};

void AudioStream::peek_into(std::size_t nb_samples, float* target) const {
    for (int i = 0; i < nb_samples * this->m_nb_channels; i++) {
        target[i] = uint8_sample_to_normalized_float(this->m_stream[this->m_playhead * this->m_nb_channels + i]);
    }
};

void AudioStream::read_into(std::size_t nb_samples, float* target)  {
    this->peek_into(nb_samples, target);
    this->advance(nb_samples);
};

void AudioStream::advance(std::size_t nb_samples) {
    this->m_playhead += nb_samples;
};

int AudioStream::get_nb_channels() const {
    return this->m_nb_channels;
};

int AudioStream::get_sample_rate() const {
    return this->m_sample_rate;
};

bool AudioStream::is_initialized() const {
    return this->m_initialized;
};

std::size_t AudioStream::get_nb_can_read() const {
    return this->get_nb_samples() - this->m_playhead;
}
