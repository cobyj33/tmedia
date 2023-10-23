#ifndef ASCII_VIDEO_W_MINIAUDIO_H
#define ASCII_VIDEO_W_MINIAUDIO_H

extern "C" {
#include <miniaudio.h>
}

class ma_device_w {
  private: 
    ma_device device;
    ma_device_config config_cache;

  public:
    ma_device_w(const ma_device_config *pConfig);

    void start();
    void stop();
    // ma_device_state get_state();

    void set_volume(double volume);


    ~ma_device_w();
};


#endif