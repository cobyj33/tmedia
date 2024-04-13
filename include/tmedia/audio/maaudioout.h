#ifndef TMEDIA_MA_AO_H
#define TMEDIA_MA_AO_H

#include <tmedia/audio/audioout.h>

#include <tmedia/audio/wminiaudio.h>

#include <readerwritercircularbuffer.h>

#include <memory>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <functional>

extern "C" {
  #include <miniaudio.h>
}

struct MAAudioOutCallbackData {
  moodycamel::BlockingReaderWriterCircularBuffer<float>* audio_queue;
  std::atomic<bool>* muted;
  MAAudioOutCallbackData(moodycamel::BlockingReaderWriterCircularBuffer<float>* audio_queue, std::atomic<bool>* muted) : audio_queue(audio_queue), muted(muted) {}
};

class MAAudioOut : public AudioOut {
  private:
    std::thread audio_queue_fill_thread;
    std::function<void(float*, int)> m_on_data;
    MAAudioOutCallbackData* m_cb_data;

    void audio_queue_fill_thread_func();
    int get_data_req_size(int max_buffer_size); // to be called from audio_queue_fill_thread only!

    const int m_nb_channels;
    std::unique_ptr<ma_device_w> m_audio_device;
    moodycamel::BlockingReaderWriterCircularBuffer<float>* m_audio_queue;
    std::atomic<bool> m_muted;

    const int m_sample_rate;
    enum class MAAudioOutState { STOPPING, STOPPED, PLAYING };
    std::atomic<MAAudioOutState> state;

    const int m_audio_queue_size_frames;
    std::condition_variable stop_cond;
    std::mutex stop_mutex;
    const int m_audio_queue_size_samples;

  public:
    MAAudioOut(int nb_channels, int sample_rate, std::function<void(float*, int)> on_data);

    bool playing() const;
    void start();
    void stop();

    double volume() const;
    bool muted() const;
    void set_volume(double volume);
    void set_muted(bool muted);
    ~MAAudioOut();
};

#endif