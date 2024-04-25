#ifndef TMEDIA_AUDIO_RING_BUFFER_H
#define TMEDIA_AUDIO_RING_BUFFER_H

#include <vector>
#include <cstddef>
#include <tmedia/util/defines.h>

/**
 * Constant-Sized Non-Self Overwriting Ring Buffer for audio. It throws errors 
 * if you try to read too much data at once, which may be atrocious. As of right
 * now, its only used through the BlockingAudioRingBuffer wrapper though.
*/
class AudioRingBuffer {
  private: 
    std::vector<float> rb;
    int m_size_frames;
    int m_size_samples;
    int m_nb_channels;

    int m_sample_rate;
    double m_start_time;
    std::size_t m_frames_read;

    int m_head;
    int m_tail;

  public:


    AudioRingBuffer(int frame_capacity, int nb_channels, int sample_rate, double playback_start_time);

    TMEDIA_ALWAYS_INLINE inline int get_nb_channels() {
      return this->m_nb_channels;
    }

    TMEDIA_ALWAYS_INLINE inline int get_sample_rate() {
      return this->m_sample_rate;
    }

    void clear(double new_start_time);

    double get_buffer_current_time();
    double get_buffer_end_time();
    bool is_time_in_bounds(double playback_time);
    void set_time_in_bounds(double playback_time);

    // Every function under this line requires holding onto a read_write_mutex
    // to be thread-safe

    int get_frames_can_read();
    int get_frames_can_write();

    void read_into(int nb_frames, float* out);

    void peek_into(int nb_frames, float* out);
    std::vector<float> peek_into(int nb_frames);

    void write_into(int nb_frames, float* in);
};

#endif