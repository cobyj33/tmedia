#include "audioout.h"
#include "maaudioout.h"

#include "wmath.h"

#include <algorithm>
#include <memory>
#include <cassert>

extern "C" {
  #include <miniaudio.h>
}

using ms = std::chrono::milliseconds;

// audio constants
static constexpr int AUDIO_QUEUE_SIZE_FRAMES = 4096;
static constexpr int FETCHER_AUDIO_READING_BLOCK_SIZE_SAMPLES = 4096;
static constexpr int MINIAUDIO_PERIOD_SIZE_FRAMES = 0;
static constexpr int MINIAUDIO_PERIOD_SIZE_MS = 75;
static constexpr int MINIAUDIO_PERIODS = 3;

// ONLY call from audio_queue_fill_thread
int MAAudioOut::get_data_req_size(int max_buffer_size) {
  static constexpr int DATA_GET_EXTEND_FACTOR = 2;

  const int q_cap = static_cast<int>(this->m_audio_queue->max_capacity());
  const int q_cap_frames = q_cap / this->m_nb_channels;
  const int queue_size = static_cast<int>(this->m_audio_queue->size_approx());
  const int queue_size_frames = queue_size / this->m_nb_channels;
  const int queue_frames_to_fill = q_cap_frames - queue_size_frames;
  const int request_size_frames = std::min(queue_frames_to_fill * DATA_GET_EXTEND_FACTOR, max_buffer_size); // fetch a little extra
  return request_size_frames;
}

void MAAudioOut::audio_queue_fill_thread_func() {
  static constexpr int AUDIO_BUFFER_READ_INTO_TRY_WAIT_MS = 10;
  static constexpr float RAMP_UP_TIME_SECONDS = 0.0625f;
  static constexpr float RAMP_DOWN_TIME_SECONDS = 0.0625f;
  float stkbuf[FETCHER_AUDIO_READING_BLOCK_SIZE_SAMPLES];
  const int stkbuf_frames_size = FETCHER_AUDIO_READING_BLOCK_SIZE_SAMPLES / this->m_nb_channels;

  /**
   * Ramping up and down is done to prevent weird sudden jumping in the speaker
   * whenever audio is started or stopped.
  */

 #if 0
 {
    const float rampup_sample_end = static_cast<float>(this->m_sample_rate) * RAMP_UP_TIME_SECONDS;
    float rampup_samples = 0;

    while (rampup_samples < rampup_sample_end) {
      const int request_size_frames = stkbuf_frames_size;
      this->m_on_data(stkbuf, request_size_frames);
      const int nb_samples = stkbuf_frames_size * this->m_nb_channels;
      for (int i = 0; i < nb_samples; i++) {
        const float vol_percent = rampup_samples / rampup_sample_end;
        const float adjusted_sample = stkbuf[i] * vol_percent;
        while (this->playing() && !this->m_audio_queue->wait_enqueue_timed(adjusted_sample, ms(AUDIO_BUFFER_READ_INTO_TRY_WAIT_MS))) {}
        rampup_samples++;
      }

    }
 }
 #endif



  while (this->state == MAAudioOutState::PLAYING) {
    const int request_size_frames = this->get_data_req_size(stkbuf_frames_size);

    this->m_on_data(stkbuf, request_size_frames);
    const int nb_samples = request_size_frames * this->m_nb_channels;
    for (int i = 0; i < nb_samples; i++) {
      while (this->playing() && !this->m_audio_queue->wait_enqueue_timed(stkbuf[i], ms(AUDIO_BUFFER_READ_INTO_TRY_WAIT_MS))) {}
    }
  } 
  // state is MAAudioOutState::STOPPING

  #if 0
  {
    // const float rampdown_sample_start = this->m_sample_rate / 0.25f; // 0.0625 seconds
    const float rampdown_sample_start = static_cast<float>(this->m_sample_rate) * RAMP_DOWN_TIME_SECONDS;
    float rampdown_samples = rampdown_sample_start;

    while (rampdown_samples > 0.0f) {
      const int request_size_frames = stkbuf_frames_size;
      this->m_on_data(stkbuf, request_size_frames);
      const int nb_samples = stkbuf_frames_size * this->m_nb_channels;
      for (int i = 0; i < nb_samples; i++) {
        const float vol_percent = rampdown_samples / rampdown_sample_start;
        const float adjusted_sample = stkbuf[i] * vol_percent;
        while (this->playing() && !this->m_audio_queue->wait_enqueue_timed(adjusted_sample, ms(AUDIO_BUFFER_READ_INTO_TRY_WAIT_MS))) {}
        rampdown_samples--;
      }
    }
  }
  #endif

  while (this->m_audio_queue->size_approx() > 0) { } // spin until queue is empty! 

  std::unique_lock<std::mutex> stop_lock(this->stop_mutex);
  this->state = MAAudioOutState::STOPPED;
  this->stop_cond.notify_all();
}

void audioOutDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  float sample;
  MAAudioOutCallbackData* cb_data = (MAAudioOutCallbackData*)pDevice->pUserData;
  bool muted = *cb_data->muted;
  const ma_uint32 sampleCount = frameCount * pDevice->playback.channels;
  for (ma_uint32 i = 0; i < sampleCount; i++) {
    sample = 0.0f;
    cb_data->audio_queue->try_dequeue(sample);
    ((float*)pOutput)[i] = sample * !muted;
  }

  (void)pInput;
}

MAAudioOut::MAAudioOut(int nb_channels, int sample_rate, std::function<void(float*, int)> on_data)
  : m_nb_channels(nb_channels), m_sample_rate(sample_rate),
  m_audio_queue_size_frames(AUDIO_QUEUE_SIZE_FRAMES),
  m_audio_queue_size_samples(AUDIO_QUEUE_SIZE_FRAMES * nb_channels) {
  assert(nb_channels > 0);
  assert(sample_rate > 0);
  this->m_muted = false;

  this->m_audio_queue = new moodycamel::BlockingReaderWriterCircularBuffer<float>(this->m_audio_queue_size_samples);

  this->m_cb_data = new MAAudioOutCallbackData(this->m_audio_queue, &this->m_muted);
  this->m_on_data = on_data;

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format  = ma_format_f32;
  config.playback.channels = nb_channels;
  config.sampleRate = sample_rate;       
  config.dataCallback = audioOutDataCallback;   
  config.noPreSilencedOutputBuffer = MA_TRUE;
  config.noClip = MA_TRUE;
  config.noFixedSizedCallback = MA_TRUE;
  config.periodSizeInFrames = MINIAUDIO_PERIOD_SIZE_FRAMES;
  config.periodSizeInMilliseconds = MINIAUDIO_PERIOD_SIZE_MS;
  config.periods = MINIAUDIO_PERIODS;
  config.pUserData = this->m_cb_data;

  this->m_audio_device = std::make_unique<ma_device_w>(&config);
}

bool MAAudioOut::playing() const {
  return this->m_audio_device->playing();
}

void MAAudioOut::start() {
  if (this->m_audio_device->playing()) return;
  this->state = MAAudioOutState::PLAYING;
  this->m_audio_device->start();
  std::thread initialized_audio_queue_fill_thread(&MAAudioOut::audio_queue_fill_thread_func, this);
  this->audio_queue_fill_thread.swap(initialized_audio_queue_fill_thread);
}

void MAAudioOut::stop() {
  if (!this->m_audio_device->playing()) return;

  constexpr long INITIAL_STOP_WAIT_DURATION = 20;
  constexpr long SUBSEQUENT_WAIT_DURATION = 20;
  
  {
    std::unique_lock<std::mutex> stop_cond_lock(this->stop_mutex);
    this->state = MAAudioOutState::STOPPING; // the only place where stopping is set
    this->stop_cond.wait_for(stop_cond_lock, ms(INITIAL_STOP_WAIT_DURATION));
    while (this->state != MAAudioOutState::STOPPED) { // wait for audio_thread_fill_func to dump all audio data
      this->stop_cond.wait_for(stop_cond_lock, ms(SUBSEQUENT_WAIT_DURATION));
    }
  }

  this->m_audio_device->stop();
  if (this->audio_queue_fill_thread.joinable())
    this->audio_queue_fill_thread.join();
  this->state = MAAudioOutState::STOPPED;
}

double MAAudioOut::volume() const {
  return this->m_audio_device->get_volume();
}

bool MAAudioOut::muted() const {
  return this->m_muted;
}

void MAAudioOut::set_volume(double volume) {
  this->m_audio_device->set_volume(clamp(volume, 0.0, 1.0));
}

void MAAudioOut::set_muted(bool muted) {
  this->m_muted = muted;
}

MAAudioOut::~MAAudioOut() {
  this->stop();
  delete this->m_audio_queue;
  delete this->m_cb_data;
}