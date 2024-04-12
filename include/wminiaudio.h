#ifndef TMEDIA_W_MINIAUDIO_H
#define TMEDIA_W_MINIAUDIO_H

extern "C" {
#include <miniaudio.h>
}

#include <atomic>

class ma_device_w {
  private: 
    ma_device device;
    ma_device_config config_cache;
    std::atomic<float> volume_cache;
    std::atomic<bool> m_playing;

  public:
    ma_device_w(const ma_device_config *pConfig);

    void start();
    void stop();
    void set_volume(double volume);
    double get_volume() const;
    bool playing() const;

    ~ma_device_w();
};


#endif