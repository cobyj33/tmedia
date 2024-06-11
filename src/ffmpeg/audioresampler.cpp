#include <tmedia/ffmpeg/audioresampler.h>

#include <tmedia/ffmpeg/avguard.h>
#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/ffmpeg/deleter.h>
#include <tmedia/util/defines.h>

#include <memory>
#include <fmt/format.h>
#include <stdexcept>

extern "C" {
  #include <libswresample/swresample.h>
  #include <libavutil/frame.h>
  #include <libavutil/channel_layout.h>
  #include <libavutil/samplefmt.h>
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
    throw ffmpeg_error(fmt::format("[{}] Allocation of internal SwrContext of "
    "AudioResampler failed. Aborting...", FUNCDINFO), result);
  }

  result = swr_init(context);
  if (result != 0) {
    swr_free(&context);
    throw ffmpeg_error(fmt::format("[{}] Initialization of internal "
    "SwrContext of AudioResampler failed. Aborting...", FUNCDINFO), result);
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
    swr_free(&context);
    throw ffmpeg_error(fmt::format("[{}] Failed to copy source "
    "AVChannelLayout data into internal field", FUNCDINFO), result);
  }

  result = av_channel_layout_copy(&this->m_dst_ch_layout, dst_ch_layout);
  if (result < 0) {
    swr_free(&context);
    throw ffmpeg_error(fmt::format("[{}] Failed to copy "
    "destination AVChannelLayout data into internal field", FUNCDINFO), result);
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

void AudioResampler::resample_audio_frame(AVFrame* dest, AVFrame* src) {
  // ! NOTE: src CAN BE NULL!!! Keep this in mind when writing code in this function
  int result;
  dest->sample_rate = this->m_dst_sample_rate;
  dest->format = this->m_dst_sample_fmt;
  #if HAS_AVCHANNEL_LAYOUT
  result = av_channel_layout_copy(&dest->ch_layout, &this->m_dst_ch_layout);
  if (unlikely(result != 0)) {
    throw ffmpeg_error(fmt::format("[{}] Unable to copy destination audio "
    "channel layout", FUNCDINFO), result);
  }
  #else
  dest->channel_layout = this->m_dst_ch_layout;
  #endif

  if (src) {
    dest->pts = src->pts;
    #if HAS_AVFRAME_DURATION
    dest->duration = src->duration;
    #endif
  }

  result = swr_convert_frame(this->m_context, dest, src);

  // .wav files were having a weird glitch where swr_conver_frame returns
  // AVERROR_INPUT_CHANGED on calling swr_convert_frame. This allows the
  // SwrContext to "fix itself" (even though I think that's a bug)
  // whenever this happens.
  // This is a trashy solution... :(
  if (unlikely(result == AVERROR_INPUT_CHANGED || result == AVERROR_OUTPUT_CHANGED)) {
    result = swr_config_frame(this->m_context, dest, src);
    if (result != 0) {
      throw ffmpeg_error(fmt::format("[{}] Unable to reconfigure "
      "resampling context", FUNCDINFO), result);
    }
    result = swr_convert_frame(this->m_context, dest, src);
  }

  if (result != 0) {
    throw ffmpeg_error(fmt::format("[{}] Unable to resample audio frame", FUNCDINFO), result);
  }
}
