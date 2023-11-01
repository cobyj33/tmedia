#ifndef TMEDIA_AUDIO_RING_BUFFER_H
#define TMEDIA_AUDIO_RING_BUFFER_H

#include <vector>
#include <cstddef>

/**
 * Constant size Non-Self Overwriting Ring Buffer Implementation
*/
class AudioRingBuffer {
  private: 
    std::vector<float> m_ring_buffer;
    int m_size_frames;
    int m_size_samples;
    int m_nb_channels;

    int m_head;
    int m_tail;
  public:


    AudioRingBuffer(int frame_capacity, int nb_channels);

    int get_nb_channels();

    void clear();

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