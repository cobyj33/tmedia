
#ifndef ASCII_VIDEO_AUDIO_STREAM_INCLUDED
#define ASCII_VIDEO_AUDIO_STREAM_INCLUDED
#include <cstdint>
#include <vector>

class AudioStream {
    private:
        std::vector<uint8_t> m_stream;
        float m_start_time;
        // size_t m_nb_samples;
        std::size_t m_playhead;
        // size_t m_sample_capacity;
        int m_nb_channels;
        int m_sample_rate;
        bool m_initialized;

    public:
        AudioStream();
        std::size_t get_nb_samples();
        void clear_and_restart_at(double time);
        void clear();
        void init(int nb_channels, int sample_rate);
        double get_time();
        double elapsed_time();
        std::size_t set_time(double time); // returns new playhead position
        double get_end_time();
        void write(uint8_t sample_part);
        void write(float sample_part);
        void write(float* samples, int nb_samples);
        bool is_time_in_bounds(double time);
        bool can_read(std::size_t nb_samples);
        void read_into(std::size_t nb_samples, float* target);
        void peek_into(std::size_t nb_samples, float* target);
        void advance(std::size_t nb_samples);

        int get_nb_channels();
        int get_sample_rate();
        bool is_initialized();
};

#endif