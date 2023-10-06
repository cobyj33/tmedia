

#include "wminiaudio.h"

#include <stdexcept>

extern "C" {
#include <miniaudio.h>
}

ma_device_w::ma_device_w(ma_context *pContext, const ma_device_config *pConfig) {
  ma_result log = ma_device_init(pContext, pConfig, &this->device);
  if (log != MA_SUCCESS) {
    throw std::runtime_error("[ma_device_w::ma_device_w] FAILED TO INITIALIZE AUDIO DEVICE: " + std::string(ma_result_description(log)));
  }

  log = ma_device_start(&this->device);
  if (log != MA_SUCCESS) {
    ma_device_uninit(&this->device);
    throw std::runtime_error("[ma_device_w::ma_device_w] Failed to initially start playback: Miniaudio Error: " + std::string(ma_result_description(log)));
  }

  log = ma_device_stop(&this->device);
  if (log != MA_SUCCESS) {
    ma_device_uninit(&this->device);
    throw std::runtime_error("[ma_device_w::ma_device_w] Failed to initially stop playback: Miniaudio Error: " + std::string(ma_result_description(log)));
  }

}

void ma_device_w::start() {
  ma_result log = ma_device_start(&this->device);
  if (log != MA_SUCCESS) {
    throw std::runtime_error("[ma_device_w::ma_device_w] Failed to start playback: Miniaudio Error: " + std::string(ma_result_description(log)));
  }
}

void ma_device_w::stop() {
  ma_result log = ma_device_stop(&this->device);
  if (log != MA_SUCCESS) {
    throw std::runtime_error("[ma_device_w::ma_device_w] Failed to stop playback: Miniaudio Error: " + std::string(ma_result_description(log)));
  }
}

ma_device_state ma_device_w::get_state() {
  return ma_device_get_state(&this->device);
}

ma_device_w::~ma_device_w() {
  ma_device_uninit(&this->device);
}
