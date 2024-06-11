#ifndef TMEDIA_AUDIO_RING_BUFFER_H
#define TMEDIA_AUDIO_RING_BUFFER_H

/**
 * Constant-Sized Non-Self-Overwriting Ring Buffer for audio.
 *
 * It is expected that whenever reading or peeking data into the
 * AudioRingBuffer, the amount
 * of data read is less than or equal to the amount given by
 * get_frames_can_read, or undefined behavior is exhibited.
 * Similarly, whenever writing data into the AudioRingBuffer, it is expected
 * that the amount of data written is less than
 * or equal to the amount given by get_frames_can_write, or undefined behavior
 * is exhibited.
 *
 * The number of channels and sample rate are set at construction time.
 *
 * AudioRingBuffer can also track the time that the first sample and last sample
 * given in the AudioRingBuffer should be at by keeping track of the sample rate
 * and the amount of frames read from the AudioRingBuffer. This time tracking
 * feature has no effect on  storing, reading, and writing audio data, and is
 * just implemented as a convenience. This tracking can be reset at any time
 * by calling clear(), which clears the audio buffer and sets a new time
 * to begin tracking how much data has been read or written at the configured
 * sample rate.
 *
 * As of right now in tmedia, its only used through the BlockingAudioRingBuffer
 * wrapper.
*/
class AudioRingBuffer {
  private:
    float* rb;
    int m_size_frames;
    int m_size_samples;
    int m_nb_channels;

    int m_sample_rate;
    double m_start_time;
    unsigned long long m_frames_read;

    int m_head;
    int m_tail;

  public:


    AudioRingBuffer(int frame_capacity, int nb_channels, int sample_rate, double playback_start_time);

    [[gnu::always_inline]] inline int get_nb_channels() {
      return this->m_nb_channels;
    }

    [[gnu::always_inline]] inline int get_sample_rate() {
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

    void write_into(int nb_frames, float* in);
    ~AudioRingBuffer();
};

#endif
