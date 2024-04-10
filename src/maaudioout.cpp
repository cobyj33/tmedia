#include "audioout.h"
#include "maaudioout.h"

#include "wmath.h"

#include <algorithm>
#include <memory>

extern "C" {
  #include <miniaudio.h>
}

// audio constants
static constexpr int AUDIO_QUEUE_SIZE_FRAMES = 4096;
static constexpr int FETCHER_AUDIO_READING_BLOCK_SIZE = 1024;
static constexpr int MINIAUDIO_PERIOD_SIZE_FRAMES = 0;
static constexpr int MINIAUDIO_PERIOD_SIZE_MS = 75;
static constexpr int MINIAUDIO_PERIODS = 3;

void MAAudioOut::audio_queue_fill_thread_func() {
  static constexpr int AUDIO_BUFFER_READ_INTO_TRY_WAIT_MS = 10;
  static constexpr int DATA_GET_EXTEND_FACTOR = 2;

  float stkbuf[FETCHER_AUDIO_READING_BLOCK_SIZE];
  const int stkbuf_frames_size = FETCHER_AUDIO_READING_BLOCK_SIZE / this->m_nb_channels;
  const int q_cap = static_cast<int>(this->m_audio_queue->max_capacity());
  const int q_cap_frames = q_cap / this->m_nb_channels;


  while (this->playing()) {
    const int queue_size = static_cast<int>(this->m_audio_queue->size_approx());
    const int queue_size_frames = queue_size / this->m_nb_channels;
    const int queue_frames_to_fill = q_cap_frames - queue_size_frames;
    const int request_size_frames = std::min(queue_frames_to_fill * DATA_GET_EXTEND_FACTOR, stkbuf_frames_size); // fetch a little extra

    this->m_on_data(stkbuf, request_size_frames);
    const int nb_samples = request_size_frames * this->m_nb_channels;
    for (int i = 0; i < nb_samples; i++) {
      while (this->playing() && !this->m_audio_queue->wait_enqueue_timed(stkbuf[i], std::chrono::milliseconds(AUDIO_BUFFER_READ_INTO_TRY_WAIT_MS))) {}
    }
  }
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

MAAudioOut::MAAudioOut(int nb_channels, int sample_rate, std::function<void(float*, int)> on_data) {
  this->m_nb_channels = nb_channels;
  this->m_sample_rate = sample_rate;
  this->m_muted = false;
  
  this->m_audio_queue = new moodycamel::BlockingReaderWriterCircularBuffer<float>(AUDIO_QUEUE_SIZE_FRAMES * nb_channels);
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
  this->m_audio_device->start();
  std::thread initialized_audio_queue_fill_thread(&MAAudioOut::audio_queue_fill_thread_func, this);
  this->audio_queue_fill_thread.swap(initialized_audio_queue_fill_thread);
}

void MAAudioOut::stop() {
  if (!this->m_audio_device->playing()) return;
  this->m_audio_device->stop();
  if (this->audio_queue_fill_thread.joinable()) this->audio_queue_fill_thread.join();
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