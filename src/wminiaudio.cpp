#include "wminiaudio.h"

#include "wmath.h"

#include <stdexcept>

extern "C" {
#include <miniaudio.h>
}

/**
 * We just completely uninitialize and reinitialize the internal ma_device on
 * start and stop because of some weird pulseaudio resync 
 * 
 * This allows so that if we can detect that the audio callback is not being
 * fired fast enough to keep up with our actual media playback, we can just
 * restart the ma_device and restore the audio playback.
 * 
 * Pretty duct taped together
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
  this->volume_cache = 1.0;
  this->m_playing = false;
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
    this->set_volume(this->volume_cache);
  }

  ma_result log = ma_device_start(&this->device);
  if (log != MA_SUCCESS) {
    throw std::runtime_error("[ma_device_w::start] Failed to start playback: Miniaudio Error: " + std::string(ma_result_description(log)));
  }

  this->m_playing = true;
}

void ma_device_w::stop() {
  ma_device_uninit(&this->device);
  this->m_playing = false;
}

bool ma_device_w::playing() const {
  return this->m_playing;
}

void ma_device_w::set_volume(double volume) {
  double clamped_volume = clamp(volume, 0.0, 1.0);
  ma_device_set_master_volume(&this->device, clamped_volume);
  this->volume_cache = clamped_volume;
}

double ma_device_w::get_volume() const {
  float res;
  ma_result err = ma_device_get_master_volume((ma_device*)&this->device, &res);
  if (err != MA_SUCCESS) {
    throw std::runtime_error("[ma_device_w::get_volume] Failed to get miniaudio volume: Miniaudio Error: " + std::string(ma_result_description(err)));
  }

  return res;
}

ma_device_w::~ma_device_w() {
  ma_device_uninit(&this->device);
}
