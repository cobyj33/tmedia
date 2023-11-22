#ifndef TMEDIA_MA_AO_H
#define TMEDIA_MA_AO_H

#include "audioout.h"

#include "wminiaudio.h"

#include "readerwritercircularbuffer.h"

#include <memory>
#include <atomic>
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

    std::unique_ptr<ma_device_w> m_audio_device;
    moodycamel::BlockingReaderWriterCircularBuffer<float>* m_audio_queue;
    std::atomic<bool> m_muted;

    int m_nb_channels;
    int m_sample_rate;
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