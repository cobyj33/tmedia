#include "mediadecoder.h"

#include "avguard.h"
#include "boiler.h"
#include "metadata.h"
#include "decode.h"
#include "formatting.h"
#include "funcmac.h"
#include "ffmpeg_error.h"

#include <string>
#include <vector>
#include <set>
#include <stdexcept> 
#include <memory> //std::make_unique
#include <utility>
#include <filesystem>

#include <fmt/format.h>

extern "C" {
  #include <libavutil/avutil.h>
  #include <libavformat/avformat.h>
}

MediaDecoder::MediaDecoder(std::filesystem::path path, std::set<enum AVMediaType>& requested_streams) {
  try {
    this->fmt_ctx = open_format_context(path);
  } catch (std::runtime_error const& e) {
    throw std::runtime_error(fmt::format("[{}] Could not allocate Media "
    "Decoder of {} because of error while fetching file format data: {}",
    FUNCDINFO, path.c_str(), + e.what()));
  }

  this->media_type = media_type_from_avformat_context(this->fmt_ctx);
  this->metadata = fmt_ctx_meta(this->fmt_ctx);

  for (const enum AVMediaType& stream_type : requested_streams) {
    try {
      this->stream_decoders[stream_type] = std::move(std::make_unique<StreamDecoder>(fmt_ctx, stream_type));
    } catch (std::runtime_error const& e) {
      continue;
    }
  }
}

MediaDecoder::~MediaDecoder() {
  avformat_close_input(&(this->fmt_ctx));
}

double MediaDecoder::get_duration() const {
  return this->fmt_ctx->duration / AV_TIME_BASE;
}

bool MediaDecoder::has_stream_decoder(enum AVMediaType media_type) const {
  return this->stream_decoders.count(media_type) == 1;
}

std::vector<AVFrame*> MediaDecoder::next_frames(enum AVMediaType media_type) {
  if (!this->has_stream_decoder(media_type)) {
    throw std::runtime_error(fmt::format("[{0}] Cannot get next {1} frames, "
    "as no {1} stream is available to decode from",
    FUNCDINFO, av_get_media_type_string(media_type)));
  }

  constexpr int NO_FETCH_MADE = -1;

  StreamDecoder& stream_decoder = *(this->stream_decoders[media_type]);
  std::vector<AVFrame*> dec_frames;
  int fetch_count = NO_FETCH_MADE; 

  do {
    fetch_count = !stream_decoder.has_packets() ? this->fetch_next(10) : NO_FETCH_MADE;
    dec_frames = stream_decoder.decode_next();
    if (dec_frames.size() > 0) {
      return dec_frames;
    }
    // no need to clear dec_frames, if the size of decoded frames is greater than 0, would have already returned
  } while (fetch_count > 0 || fetch_count == NO_FETCH_MADE);

  return dec_frames; // no video frames could sadly be found. This should only really ever happen once the file ends
}

int MediaDecoder::fetch_next(int requested_packet_count) {
  int packets_read = 0;
  AVPacket* reading_packet = av_packet_alloc();
  if (reading_packet == nullptr) {
    throw ffmpeg_error(fmt::format("[{}] Failed to allocate AVPacket", FUNCDINFO), AVERROR(ENOMEM));
  }

  while (av_read_frame(this->fmt_ctx, reading_packet) == 0) {
     
    for (auto &decoder_pair : this->stream_decoders) {
      if (decoder_pair.second->get_stream_index() == reading_packet->stream_index) {
        AVPacket* saved_packet = av_packet_alloc();
        if (saved_packet == nullptr) {
          av_packet_free(&reading_packet);
          throw ffmpeg_error(fmt::format("[{}] Failed to allocate AVPacket", FUNCDINFO), AVERROR(ENOMEM));
        }

        av_packet_ref(saved_packet, reading_packet);
        
        decoder_pair.second->push_back(saved_packet);
        packets_read++;
        break;
      }
    }

    av_packet_unref(reading_packet);

     if (packets_read >= requested_packet_count) break;
  }

  //reached EOF
  av_packet_free(&reading_packet);
  return packets_read;
}

int MediaDecoder::jump_to_time(double target_time) {
  if (target_time < 0.0 || target_time > this->get_duration()) {
    throw std::runtime_error(fmt::format("[{}]Could not jump to time {} ({} "
    "seconds), time is out of the bounds of duration {} ({} seconds)",
    FUNCDINFO, format_time_hh_mm_ss(target_time), target_time,
    format_time_hh_mm_ss(this->get_duration()), this->get_duration()));
  }

  int ret = avformat_seek_file(this->fmt_ctx, -1, 0.0,
    target_time * AV_TIME_BASE, target_time * AV_TIME_BASE, 0);

  if (ret < 0) {
    return ret;
  }

  for (auto& decoder_pair : this->stream_decoders) {
    decoder_pair.second->reset();
  }

  for (auto& decoder_pair : this->stream_decoders) {
    std::vector<AVFrame*> frames;
    bool passed_target_time = false;

    do {
      clear_avframe_list(frames);
      frames = this->next_frames(decoder_pair.first);
      for (std::size_t i = 0; i < frames.size(); i++) {
        if (frames[i]->pts * decoder_pair.second->get_time_base() >= target_time) {
          passed_target_time = true;
          break;
        }
      }
    } while (!passed_target_time && frames.size() > 0);
    
    clear_avframe_list(frames);
  }

  return ret;
}

MediaType MediaDecoder::get_media_type() {
  return this->media_type;
}

/**
 * 
 * Audio Getters 
 * 
*/

int MediaDecoder::get_nb_channels() {
  if (!this->has_stream_decoder(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get number of audio "
    "channels from this media decoder: No available audio stream decoder.",
    FUNCDINFO));
  }

  #if HAS_AVCHANNEL_LAYOUT
  return this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->ch_layout.nb_channels;
  #else
  return this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channels;
  #endif
}

int MediaDecoder::get_sample_rate() {
  if (!this->has_stream_decoder(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get sample "
    "rate from this media decoder: No audio stream decoder is available",
    FUNCDINFO));
  }

  return this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->sample_rate;
}

AVSampleFormat MediaDecoder::get_sample_fmt() {
  if (!this->has_stream_decoder(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get sample "
    "format from this media decoder: No audio stream decoder is available",
    FUNCDINFO));
  }

  return this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->sample_fmt;
}

#if HAS_AVCHANNEL_LAYOUT
AVChannelLayout* MediaDecoder::get_ch_layout() {
  if (!this->has_stream_decoder(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get channel "
    "layout from this media decoder: No audio stream decoder is available",
    FUNCDINFO));
  }

  return &(this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->ch_layout);
}
#else
int64_t MediaDecoder::get_ch_layout() {
  if (!this->has_stream_decoder(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get channel "
    "layout from this media decoder: No audio stream decoder is available",
    FUNCDINFO));
  }

  int64_t channel_layout = this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channel_layout;
  if (channel_layout != 0) return channel_layout;

  int channels = this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channels;
  if (channels >= 0) return av_get_default_channel_layout(channels);
  return AV_CH_LAYOUT_STEREO;
}
#endif


/**
 * 
 * Video Getters 
 * 
*/

int MediaDecoder::get_width() {
  if (!this->has_stream_decoder(AVMEDIA_TYPE_VIDEO)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get width "
    "layout from this media decoder: No video stream decoder is available",
    FUNCDINFO));
  }

  return this->stream_decoders[AVMEDIA_TYPE_VIDEO]->get_codec_context()->width;
}

int MediaDecoder::get_height() {
  if (!this->has_stream_decoder(AVMEDIA_TYPE_VIDEO)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get height "
    "layout from this media decoder: No video stream decoder is available",
    FUNCDINFO));
  }

  return this->stream_decoders[AVMEDIA_TYPE_VIDEO]->get_codec_context()->height;
}

AVPixelFormat MediaDecoder::get_pix_fmt() {
  if (!this->has_stream_decoder(AVMEDIA_TYPE_VIDEO)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get pixel "
    "format from this media decoder: No video stream decoder is available",
    FUNCDINFO));
  }

  return this->stream_decoders[AVMEDIA_TYPE_VIDEO]->get_codec_context()->pix_fmt;
}

/**
 * 
 * Miscellaneous Getters 
 * 
*/

double MediaDecoder::get_start_time(enum AVMediaType media_type) {
  if (!this->has_stream_decoder(media_type)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get start "
    "time from this media decoder: No {} stream decoder is available",
    FUNCDINFO, av_get_media_type_string(media_type)));
  }

  return this->stream_decoders[media_type]->get_start_time();
}

double MediaDecoder::get_time_base(enum AVMediaType media_type) {
  if (!this->has_stream_decoder(media_type)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get time "
    "base from this media decoder: No {} stream decoder is available",
    FUNCDINFO, av_get_media_type_string(media_type)));
  }

  return this->stream_decoders[media_type]->get_time_base();

}

double MediaDecoder::get_avgfts(enum AVMediaType media_type) {
  if (!this->has_stream_decoder(media_type)) {
    throw std::runtime_error(fmt::format("[{}] Cannot get average "
    "frame time sec from this media decoder: No {} stream decoder is available",
    FUNCDINFO, av_get_media_type_string(media_type)));
  }

  return this->stream_decoders[media_type]->get_avgfts();
}