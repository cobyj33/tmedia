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
    BlockingAudioRingBuffer(int frame_capcity, int nb_channels, int sample_rate, double playback_start_time);

    int get_nb_channels();
    int get_sample_rate();
    void clear(double current_playback_time);

    double get_buffer_current_time();
    bool is_time_in_bounds(double playback_time);
    bool try_set_time_in_bounds(double playback_time, int milliseconds);


    void read_into(int nb_frames, float* out);
    bool try_read_into(int nb_frames, float* out, int milliseconds);

    void peek_into(int nb_frames, float* out);
    bool try_peek_into(int nb_frames, float* out, int milliseconds);
    
    void write_into(int nb_frames, float* in);
    bool try_write_into(int nb_frames, float* in, int milliseconds);
};

#endif