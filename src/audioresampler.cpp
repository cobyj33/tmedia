#include "audioresampler.h"

#include "avguard.h"
#include "ffmpeg_error.h"

#include <stdexcept>

extern "C" {
  #include <libswresample/swresample.h>
  #include <libavutil/frame.h>
  #include <libavutil/version.h>
}


#if HAS_AVCHANNEL_LAYOUT
AudioResampler::AudioResampler(AVChannelLayout* dst_ch_layout, enum AVSampleFormat dst_sample_fmt, int dst_sample_rate, AVChannelLayout* src_ch_layout, enum AVSampleFormat src_sample_fmt, int src_sample_rate) {
#else
AudioResampler::AudioResampler(int64_t dst_ch_layout, enum AVSampleFormat dst_sample_fmt, int dst_sample_rate, int64_t src_ch_layout, enum AVSampleFormat src_sample_fmt, int src_sample_rate) {
#endif
  SwrContext* context = nullptr;
  int result = 0;
  #if HAS_AVCHANNEL_LAYOUT
  result = swr_alloc_set_opts2(
  &context, dst_ch_layout, dst_sample_fmt, 
  dst_sample_rate, src_ch_layout, src_sample_fmt,
  src_sample_rate, 0, nullptr);
  #else
  context = swr_alloc_set_opts(nullptr, dst_ch_layout, dst_sample_fmt, 
  dst_sample_rate, src_ch_layout, src_sample_fmt,
  src_sample_rate, 0, nullptr);
  #endif
  
  if (result < 0) {
    if (context != nullptr) {
      swr_free(&context);
    }
    throw ffmpeg_error("[AudioResampler::AudioResampler] Allocation of internal SwrContext of AudioResampler failed. Aborting...", result);
  } else if (context == nullptr) {
    throw std::runtime_error("[AudioResampler::AudioResampler] Allocation of internal SwrContext of AudioResampler failed. Aborting...");
  } else {
    result = swr_init(context);
    if (result < 0) {
      if (context != nullptr) {
        swr_free(&context);
      }
      throw ffmpeg_error("[AudioResampler::AudioResampler] Initialization of internal SwrContext of AudioResampler failed. Aborting...", result);
    }
  }

  this->m_context = context;
  this->m_src_sample_rate = src_sample_rate;
  this->m_src_sample_fmt = src_sample_fmt;
  this->m_dst_sample_rate = dst_sample_rate;
  this->m_dst_sample_fmt = dst_sample_fmt;

  #if HAS_AVCHANNEL_LAYOUT
  av_channel_layout_default(&this->m_src_ch_layout, 2);
  av_channel_layout_default(&this->m_dst_ch_layout, 2);

  result = av_channel_layout_copy(&this->m_src_ch_layout, src_ch_layout);
  if (result < 0) {
    throw ffmpeg_error("[AudioResampler::AudioResampler] Failed to copy source AVChannelLayout data into internal field", result);
  }

  result = av_channel_layout_copy(&this->m_dst_ch_layout, dst_ch_layout);
  if (result < 0) {
    throw ffmpeg_error("[AudioResampler::AudioResampler] Failed to copy destination AVChannelLayout data into internal field", result);
  }
  #else
  this->m_dst_ch_layout = dst_ch_layout;
  this->m_src_ch_layout = src_ch_layout;
  #endif

}



AudioResampler::~AudioResampler() {
  swr_free(&(this->m_context));
  #if HAS_AVCHANNEL_LAYOUT
  av_channel_layout_uninit(&this->m_dst_ch_layout);
  av_channel_layout_uninit(&this->m_src_ch_layout);
  #endif
}

std::vector<AVFrame*> AudioResampler::resample_audio_frames(std::vector<AVFrame*>& originals) {
  std::vector<AVFrame*> resampled_frames;
  for (int i = 0; i < (int)originals.size(); i++) {
    AVFrame* resampled_frame = this->resample_audio_frame(originals[i]);
    resampled_frames.push_back(resampled_frame);
  }

  return resampled_frames;
}


AVFrame* AudioResampler::resample_audio_frame(AVFrame* original) {
    int result;
    AVFrame* resampled_frame = av_frame_alloc();
    if (resampled_frame == NULL) {
      throw std::runtime_error("Could not create AVFrame audio frame for resampling");
    }

    resampled_frame->sample_rate = this->m_dst_sample_rate;
    resampled_frame->format = this->m_dst_sample_fmt;
    #if HAS_AVCHANNEL_LAYOUT
    result = av_channel_layout_copy(&resampled_frame->ch_layout, &this->m_dst_ch_layout);
    if (result < 0) {
      throw ffmpeg_error("[AudioResampler::resample_audio_frame] Unable to copy destination audio channel layout", result);
    }
    #else 
    resampled_frame->channel_layout = this->m_dst_ch_layout;
    #endif

    resampled_frame->pts = original->pts;
    #if HAS_AVFRAME_DURATION
    resampled_frame->duration = original->duration;
    #endif

    result = swr_convert_frame(this->m_context, resampled_frame, original);
    switch (result) {
      // .wav files were having a weird glitch where swr_conver_frame returns AVERROR_INPUT_CHANGED
      // on calling swr_convert_frame. This allows the SwrContext to "fix itself" (even though I think that's a bug)
      // whenever this happens.
      case AVERROR_INPUT_CHANGED: {
        result = swr_config_frame(this->m_context, resampled_frame, original);
        if (result != 0) {
          throw ffmpeg_error("[AudioResampler::resample_audio_frame] Unable to reconfigure resampling context", result);
        }

        result = swr_convert_frame(this->m_context, resampled_frame, original);
      }
    }

    if (result == 0) {
      return resampled_frame;
    }

    if (resampled_frame != nullptr) {
      av_frame_free(&resampled_frame);
    }
    throw ffmpeg_error("[AudioResampler::resample_audio_frame] Unable to resample audio frame", result);
}

int AudioResampler::get_src_sample_rate() {
  return this->m_src_sample_rate;
}

int AudioResampler::get_src_sample_fmt() {
  return this->m_src_sample_fmt;
}

int AudioResampler::get_dst_sample_rate() {
  return this->m_dst_sample_rate;
}

int AudioResampler::get_dst_sample_fmt() {
  return this->m_dst_sample_fmt;
}
