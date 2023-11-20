#ifndef TMEDIA_W_MINIAUDIO_H
#define TMEDIA_W_MINIAUDIO_H

extern "C" {
#include <miniaudio.h>
}

class ma_device_w {
  private: 
    ma_device device;
    ma_device_config config_cache;
    double volume_cache;

  public:
    ma_device_w(const ma_device_config *pConfig);

    void start();
    void stop();
    void set_volume(double volume);

    ~ma_device_w();
};


#endif