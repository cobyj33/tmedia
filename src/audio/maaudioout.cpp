#include <tmedia/audio/maaudioout.h>

#include <tmedia/util/unitconvert.h>
#include <tmedia/util/thread.h>

#include <algorithm>
#include <memory>
#include <cassert>
#include <sstream>
extern "C" {
  #include <miniaudio.h>
}

using ms = std::chrono::milliseconds;

// audio constants
static constexpr int AUDIO_QUEUE_SIZE_FRAMES = 2048;
static constexpr int FETCHER_AUDIO_READING_BLOCK_SIZE_SAMPLES = 4096;
static constexpr int MINIAUDIO_PERIOD_SIZE_MS = 20;
static constexpr int MINIAUDIO_PERIODS = 3;
static constexpr int MAX_CHANNELS = 64; // inclusive

// ONLY call from audio_queue_fill_thread
int MAAudioOut::get_data_req_size(int max_buffer_size) {
  static constexpr int DATA_GET_EXTEND_FACTOR = 2;

  const int queue_size = static_cast<int>(this->m_audio_queue->size_approx());
  const int queue_size_frames = queue_size / this->m_nb_channels;
  const int queue_frames_to_fill = this->m_audio_queue_cap_frames - queue_size_frames;
  const int request_size_frames = std::min(queue_frames_to_fill * DATA_GET_EXTEND_FACTOR, max_buffer_size); // fetch a little extra
  return request_size_frames;
}

#define ZERO_CROSS(sample1, sample2) (((sample1) <= 0.0f && (sample2) >= 0.0f) || ((sample1) >= 0.0f && (sample2) <= 0.0f))
#define SAMPLE(frame, channels, channel) (((frame) * (channels)) + (channel))

void MAAudioOut::audio_queue_fill_thread_func() {
  name_current_thread("tmedia_audqfill");
  static constexpr ms AUDBUF_READ_TRY_WAIT_CHRONO_MS = ms(10);
  static constexpr int RAMP_UP_TIME_MS = MINIAUDIO_PERIOD_SIZE_MS * MINIAUDIO_PERIODS * 2;
  static constexpr int RAMP_DOWN_TIME_MS = MINIAUDIO_PERIOD_SIZE_MS * MINIAUDIO_PERIODS * 2;
  static constexpr float RAMP_UP_TIME_SECONDS = static_cast<float>(RAMP_UP_TIME_MS) * MILLISECONDS_TO_SECONDS;
  static constexpr float RAMP_DOWN_TIME_SECONDS = static_cast<float>(RAMP_DOWN_TIME_MS) * MILLISECONDS_TO_SECONDS;

  float stkbuf[FETCHER_AUDIO_READING_BLOCK_SIZE_SAMPLES];
  const int nb_channels = this->m_nb_channels;
  const int stkbuf_frames_size = FETCHER_AUDIO_READING_BLOCK_SIZE_SAMPLES / nb_channels;

  /**
   * Ramping up and down is done to prevent weird sudden jumping in the speaker
   * whenever audio is started or stopped.
  */

 // i am so sorry that this code is ugly

  /**
   * Starts the audio at the first zero crossing found in the given audio stream
  */
 {
    float ch_gain[MAX_CHANNELS + 1]; // init to 0.0f
    const int rampup_frames_end = static_cast<float>(this->m_sample_rate) * RAMP_UP_TIME_SECONDS;
    int rampup_frames = 0;

    while (rampup_frames < rampup_frames_end && state == MAAudioOutState::PLAYING) {
      const int request_size_frames = this->get_data_req_size(stkbuf_frames_size);
      this->m_on_data(stkbuf, request_size_frames);

      for (int frame = 1; frame < request_size_frames; frame++) {
        for (int ch = 0; ch < nb_channels; ch++) {
          const float last_sample = stkbuf[SAMPLE(frame - 1, nb_channels, ch)];
          const float curr_sample = stkbuf[SAMPLE(frame, nb_channels, ch)];
          if (ZERO_CROSS(last_sample, curr_sample)) ch_gain[ch] = 1.0f;
          while (!this->m_audio_queue->wait_enqueue_timed(curr_sample * ch_gain[ch], AUDBUF_READ_TRY_WAIT_CHRONO_MS)) {}
        }
      }

      rampup_frames += request_size_frames;
    }
 }

  while (this->state == MAAudioOutState::PLAYING) {
    const int request_size_frames = this->get_data_req_size(stkbuf_frames_size);
    this->m_on_data(stkbuf, request_size_frames);
    const int nb_samples = request_size_frames * nb_channels;
    for (int s = 0; s < nb_samples; s++) {
      while (!this->m_audio_queue->wait_enqueue_timed(stkbuf[s], AUDBUF_READ_TRY_WAIT_CHRONO_MS)) {}
    }
  } 
  // state is MAAudioOutState::STOPPING

  /**
   * Cuts off the audio at the first zero crossing found in the given audio stream
  */
  {
    float ch_gain[MAX_CHANNELS + 1];
    for (int i = 0; i < MAX_CHANNELS + 1; i++) ch_gain[i] = 1.0f;

    // const int rampdown_frame_start = 44800 * 3; // the maximum amount of samples to look for an end
    const int rampdown_frame_start = static_cast<float>(this->m_sample_rate) * RAMP_DOWN_TIME_SECONDS; // the maximum amount of samples to look for an end
    int rampdown_frames = rampdown_frame_start;

    // while (rampdown_samples > 0.0f && !found_all_zero_crosses) {
    while (rampdown_frames > 0) {
      const int request_size_frames = this->get_data_req_size(stkbuf_frames_size);
      this->m_on_data(stkbuf, request_size_frames);

      for (int frame = 1; frame < request_size_frames; frame++) { // should this just end once we find all zero crossings?
        // found_all_zero_crosses = true;
        for (int ch = 0; ch < nb_channels; ch++) {
          const float last_sample = stkbuf[SAMPLE(frame - 1, nb_channels, ch)];
          const float curr_sample = stkbuf[SAMPLE(frame, nb_channels, ch)];
          if (ZERO_CROSS(last_sample, curr_sample)) ch_gain[ch] = 0.0f;
          while (!this->m_audio_queue->wait_enqueue_timed(curr_sample * ch_gain[ch], AUDBUF_READ_TRY_WAIT_CHRONO_MS)) {}
        }
      }

      rampdown_frames -= request_size_frames;
    }
  }

  // wait for the audio queue to drain
  while (this->m_audio_queue->size_approx() > 0) { }

  std::unique_lock<std::mutex> stop_lock(this->stop_mutex);
  this->state = MAAudioOutState::STOPPED;
  this->stop_cond.notify_all();
}

void audioOutDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  float sample = 0.0f;
  MAAudioOutCallbackData* cb_data = (MAAudioOutCallbackData*)pDevice->pUserData;
  const ma_uint32 sampleCount = frameCount * pDevice->playback.channels;
  bool muted = *cb_data->muted;
  for (ma_uint32 i = 0; i < sampleCount; i++) {
    sample = 0.0f;
    cb_data->audio_queue->try_dequeue(sample);
    ((float*)pOutput)[i] = sample * !muted;
  }

  (void)pInput;
}

MAAudioOut::MAAudioOut(int nb_channels, int sample_rate, std::function<void(float*, int)> on_data)
  : m_nb_channels(nb_channels), m_sample_rate(sample_rate),
  m_audio_queue_cap_frames(AUDIO_QUEUE_SIZE_FRAMES),
  m_audio_queue_cap_samples(AUDIO_QUEUE_SIZE_FRAMES * nb_channels) {
  assert(nb_channels > 0);
  assert(nb_channels <= MAX_CHANNELS);
  assert(sample_rate > 0);
  this->m_muted = false;

  this->m_audio_queue = new moodycamel::BlockingReaderWriterCircularBuffer<float>(this->m_audio_queue_cap_samples);
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
  config.periodSizeInFrames = 0; // we opt to use milliseconds instead
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
  std::thread initialized_audio_queue_fill_thread(&MAAudioOut::audio_queue_fill_thread_func, this);
  this->audio_queue_fill_thread.swap(initialized_audio_queue_fill_thread);
  while (this->m_audio_queue->size_approx() != this->m_audio_queue->max_capacity()) { } 
  this->m_audio_device->start();
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
  this->m_audio_device->set_volume(std::clamp<double>(volume, 0.0, 1.0));
}

void MAAudioOut::set_muted(bool muted) {
  this->m_muted = muted;
}

MAAudioOut::~MAAudioOut() {
  this->stop();
  delete this->m_audio_queue;
  delete this->m_cb_data;
}