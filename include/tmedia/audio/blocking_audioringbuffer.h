#ifndef TMEDIA_BLOCKING_AUDIO_RING_BUFFER_H
#define TMEDIA_BLOCKING_AUDIO_RING_BUFFER_H

#include <tmedia/audio/audioringbuffer.h>
#include <tmedia/util/defines.h>

#include <mutex>
#include <condition_variable>
#include <memory>

/**
 * Blocking wrapper of the AudioRingBuffer class.
 * For multithreaded access handled through locks and conditional variables.
 * 
 * Note that this should not be used in real-time threads such as audio output 
 * callback threads, but rather for general audio buffes that communicate
 * between non-real-time threads.
 * 
 * Generally, there should only be one producer and one consumer
 * for each BlockingAudioRingBuffer.
*/
class BlockingAudioRingBuffer {
  private:
    std::unique_ptr<AudioRingBuffer> rb;

    std::mutex mutex;
    std::condition_variable cond;

  public:
    BlockingAudioRingBuffer(int frame_capcity, int nb_channels, int sample_rate, double playback_start_time);
    
    // Thread safe without locking: nb_channels is read-only
    TMEDIA_ALWAYS_INLINE inline int get_nb_channels() {
      return this->rb->get_nb_channels();
    }
    
    // Thread safe without locking: nb_channels is read-only
    TMEDIA_ALWAYS_INLINE inline int get_sample_rate() {
      return this->rb->get_sample_rate();
    }


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