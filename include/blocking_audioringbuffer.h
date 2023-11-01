#ifndef TMEDIA_BLOCKING_AUDIO_RING_BUFFER_H
#define TMEDIA_BLOCKING_AUDIO_RING_BUFFER_H

#include "audioringbuffer.h"

#include <mutex>
#include <condition_variable>
#include <memory>

class BlockingAudioRingBuffer {
  private:
    std::unique_ptr<AudioRingBuffer> m_ring_buffer;

    std::mutex mutex;
    std::condition_variable cond;

  public:
    BlockingAudioRingBuffer(int size, int nb_channels);

    int get_nb_channels();

    void clear();

    void read_into(int nb_frames, float* out);
    bool try_read_into(int nb_frames, float* out, int milliseconds);

    void peek_into(int nb_frames, float* out);
    std::vector<float> peek_into(int nb_frames);
    std::vector<float> try_peek_into(int nb_frames, int milliseconds);

    void write_into(int nb_frames, float* in);
    bool try_write_into(int nb_frames, float* in, int milliseconds);
};

#endif