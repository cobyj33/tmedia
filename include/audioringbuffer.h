#ifndef ASCII_VIDEO_AUDIO_RING_BUFFER_H
#define ASCII_VIDEO_AUDIO_RING_BUFFER_H

#include <vector>
#include <cstddef>
#include <mutex>

/**
 * Non-Self Overwriting Ring Buffer Implementation
*/
class AudioRingBuffer {
  private: 
    std::vector<float> m_ring_buffer;
    int m_size;
    int m_nb_channels;
    int m_sample_rate;

    int m_head;
    int m_tail;

    void advance(std::size_t nb_frames);
  public:
    std::mutex read_write_mutex;

    AudioRingBuffer(int size, int nb_channels, int sample_rate);

    int get_nb_channels();
    int get_sample_rate();

    void clear();

    // Every function under this line requires holding onto the read_write_mutex
    // to be thread-safe

    int get_nb_can_read();
    int get_nb_can_write();

    void read_into(int nb_frames, float* out);

    void peek_into(int nb_frames, float* out);
    std::vector<float> peek_into(int nb_frames);

    void write_into(int nb_frames, float* in);
};

#endif