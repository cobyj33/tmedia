#ifndef ASCII_VIDEO_W_MINIAUDIO_H
#define ASCII_VIDEO_W_MINIAUDIO_H

extern "C" {
#include <miniaudio.h>
}

class ma_device_w {
  private: 
    ma_device device;

    ma_context* pContext;
    const ma_device_config *pConfig;

  public:
    ma_device_w(ma_context *pContext, const ma_device_config *pConfig);

    void start();
    void stop();
    ma_device_state get_state();
    ~ma_device_w();
};


#endif