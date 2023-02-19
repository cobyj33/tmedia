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

std::size_t AudioStream::get_nb_samples() {
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

double AudioStream::get_elapsed_time() {
    if (this->m_sample_rate == 0) {
        return 0.0;
    }

    return (double)this->m_playhead / (double)this->m_sample_rate;
};

double AudioStream::get_time() {
    return this->m_start_time + this->get_elapsed_time();
};

std::size_t AudioStream::set_time(double time) {
    if (this->get_nb_samples() == 0) return 0.0;
    this->m_playhead = (std::size_t)(std::min(this->get_nb_samples() - 1, std::max( (std::size_t)0, (std::size_t)(time - this->m_start_time) * this->m_sample_rate  )  )  );
    return this->m_playhead;
};

double AudioStream::get_end_time() {
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

bool AudioStream::is_time_in_bounds(double time) {
    return time >= this->m_start_time && time <= this->get_end_time();
};

void AudioStream::clear_and_restart_at(double time) {
    this->clear();
    this->m_start_time = time;
};


bool AudioStream::can_read(std::size_t nb_samples) {
    return this->m_playhead + nb_samples <= this->get_nb_samples();
};

bool AudioStream::can_read() {
    return this->can_read(1);
};

void AudioStream::peek_into(std::size_t nb_samples, float* target) {
    for (int i = 0; i < nb_samples * this->m_nb_channels; i++) {
        target[i] = uint8_sample_to_normalized_float(this->m_stream[this->m_playhead * this->m_nb_channels + i]);
    }
};

void AudioStream::read_into(std::size_t nb_samples, float* target) {
    this->peek_into(nb_samples, target);
    this->advance(nb_samples);
};

void AudioStream::advance(std::size_t nb_samples) {
    this->m_playhead += nb_samples;
};

int AudioStream::get_nb_channels() {
    return this->m_nb_channels;
};

int AudioStream::get_sample_rate() {
    return this->m_sample_rate;
};

bool AudioStream::is_initialized() {
    return this->m_initialized;
};

std::size_t AudioStream::get_nb_can_read() {
    return this->get_nb_samples() - this->m_playhead;
}


// AudioStream* audio_stream_alloc() {
//     AudioStream* audio_stream = (AudioStream*)malloc(sizeof(AudioStream));
//     if (audio_stream == NULL) {
//         return NULL;
//     }
//     audio_stream->stream = NULL;
//     audio_stream->playhead = 0;
//     audio_stream->nb_channels = 0;
//     audio_stream->start_time = 0.0;
//     audio_stream->sample_capacity = 0;
//     audio_stream->nb_samples = 0;
//     audio_stream->sample_rate = 0;
//     return audio_stream;
// }


// int audio_stream_init(AudioStream* stream, int nb_channels, int initial_size, int sample_rate) {
//     if (stream->stream != NULL) {
//         free(stream->stream);
//     }

//     stream->stream = (uint8_t*)malloc(sizeof(uint8_t) * nb_channels * initial_size * 2);
//     if (stream->stream == NULL) {
//         return 0;
//     }

//     stream->sample_capacity = initial_size * 2;
//     stream->nb_samples = 0;
//     stream->nb_channels = nb_channels;
//     stream->start_time = 0.0;
//     stream->sample_rate = sample_rate;
//     stream->playhead = 0;
//     return 1;
// }

// int audio_stream_clear(AudioStream* stream, int cleared_capacity) {
//     if (stream->stream != NULL) {
//         free(stream->stream);
//     }

//     uint8_t* tmp =  (uint8_t*)malloc(sizeof(uint8_t) * stream->nb_channels * cleared_capacity * 2);
//     if (tmp == NULL) {
//         return 0;
//     }
//     stream->stream = tmp;
//     stream->nb_samples = 0;
//     stream->playhead = 0;
//     stream->sample_capacity = cleared_capacity;
//     stream->playhead = 0;
//     return 1;
// }

// double audio_stream_time(AudioStream* stream) {
//     return stream->start_time + ((double)stream->playhead / stream->sample_rate);
// }

// double audio_stream_set_time(AudioStream* stream, double time) {
//     if (stream->nb_samples == 0) return 0.0;
//     stream->playhead = (size_t)(std::min(stream->nb_samples - 1, std::max( 0.0, (time - stream->start_time) * stream->sample_rate  )  )  );
//     return stream->playhead;
// }

// double audio_stream_end_time(AudioStream *stream) {
//     return stream->start_time + ((double)stream->nb_samples / stream->sample_rate);
// }
