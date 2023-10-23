#include "wminiaudio.h"

#include "wmath.h"

#include <stdexcept>

extern "C" {
#include <miniaudio.h>
}

/**
 * There seems to be some sort of miniaudio glitch where stopping and starting
 * audio with a gap of around 10 seconds in between the calls makes the audio
 * device playback get corrupted somehow? I don't know, but definitely note
 * that a work around for now is just to write null data to the device callback
 * instead of actually stopping the device
 * 
 * It seems to be a problem with ALSA, but maybe it's a problem with miniaudio,
 * or maybe it's a problem with Windows Subsystem for Linux. That's why we have this
 * weird getaround with the uninitializing and reinitializing of audio devices
 * 
 * Don't delete the commented start and stop functions, as they SHOULD work, and
 * I need to test them more on different environments.
*/

ma_device_w::ma_device_w(const ma_device_config *pConfig) {
  if (pConfig == nullptr) {
    throw std::invalid_argument("[ma_device_w::ma_device_w] config must not be null");
  }

  ma_result log = ma_device_init(nullptr, pConfig, &this->device);
  if (log != MA_SUCCESS) {
    throw std::runtime_error("[ma_device_w::ma_device_w] Failed to initialize audio device: " + std::string(ma_result_description(log)));
  }
  this->config_cache = *pConfig;
}

// void ma_device_w::start() {
//   ma_result log = ma_device_start(&this->device);
//   if (log != MA_SUCCESS) {
//     throw std::runtime_error("[ma_device_w::ma_device_w] Failed to start playback: Miniaudio Error: " + std::string(ma_result_description(log)));
//   }
// }

// void ma_device_w::stop() {
//   ma_result log = ma_device_stop(&this->device);
//   if (log != MA_SUCCESS) {
//     throw std::runtime_error("[ma_device_w::ma_device_w] Failed to stop playback: Miniaudio Error: " + std::string(ma_result_description(log)));
//   }
// }

void ma_device_w::start() {
  ma_device_state device_state = ma_device_get_state(&this->device);
  if (device_state == ma_device_state_uninitialized) {
    ma_result log = ma_device_init(nullptr, &this->config_cache, &this->device);
    if (log != MA_SUCCESS) {
      throw std::runtime_error("[ma_device_w::start] Failed to reinitialize audio device: " + std::string(ma_result_description(log)));
    }
  }

  ma_result log = ma_device_start(&this->device);
  if (log != MA_SUCCESS) {
    throw std::runtime_error("[ma_device_w::start] Failed to start playback: Miniaudio Error: " + std::string(ma_result_description(log)));
  }
}

void ma_device_w::stop() {
  ma_device_uninit(&this->device);
}

/**
 * Disabled for now. It won't be accurate because of the implementation of start and stop, which
 * would make this return ma_device_state_uninitialized when stopped
*/
// ma_device_state ma_device_w::get_state() {
//   return ma_device_get_state(&this->device);
// }

void ma_device_w::set_volume(double volume) {
  ma_device_set_master_volume(&this->device, clamp(volume, 0.0, 1.0));
}

ma_device_w::~ma_device_w() {
  ma_device_uninit(&this->device);
}
