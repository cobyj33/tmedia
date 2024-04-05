#include "streamdecoder.h"


#include "decode.h"
#include "boiler.h"
#include "ffmpeg_error.h"
#include "avguard.h"
#include "funcmac.h"

#include <fmt/format.h>

#include <vector>
#include <stdexcept>
#include <string>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

StreamDecoder::StreamDecoder(AVFormatContext* fmt_ctx, enum AVMediaType media_type) {
  #if AV_FIND_BEST_STREAM_CONST_DECODER
  const AVCodec* decoder;
  #else
  AVCodec* decoder;
  #endif
  
  int stream_index = -1;
  stream_index = av_find_best_stream(fmt_ctx, media_type, -1, -1, &decoder, 0);
  if (stream_index < 0) {
    throw ffmpeg_error(fmt::format("[{}] Cannot find media type for "
    "type: {}", FUNCDINFO, av_get_media_type_string(media_type)), stream_index);
  }

  this->decoder = decoder;
  this->stream = fmt_ctx->streams[stream_index];
  this->codec_context = avcodec_alloc_context3(this->decoder);

  if (this->codec_context == nullptr) {
    throw std::runtime_error(fmt::format("[{}] Could not alloc codec context "
    "from decoder: {}", FUNCDINFO, decoder->long_name));
  }

  this->media_type = media_type;

  int result = avcodec_parameters_to_context(this->codec_context, this->stream->codecpar);
  if (result < 0) {
    avcodec_free_context(&this->codec_context);
    throw ffmpeg_error(fmt::format("[{}] Could not move AVCodec "
    "parameters into context",
    FUNCDINFO), result);
  }

  result = avcodec_open2(this->codec_context, this->decoder, NULL);
  if (result < 0) {
    avcodec_free_context(&this->codec_context);
    throw ffmpeg_error(fmt::format("[{}] Could not initialize "
    "AVCodecContext with AVCodec decoder",
    FUNCDINFO), result);
  }
};

double StreamDecoder::get_average_frame_rate_sec() const noexcept {
  return av_q2d(this->stream->avg_frame_rate);
}

double StreamDecoder::get_avgfts() const noexcept {
  return 1 / av_q2d(this->stream->avg_frame_rate);
}

double StreamDecoder::get_start_time() const noexcept {
  if (this->stream->start_time == AV_NOPTS_VALUE)
    return 0.0;
  
  return this->stream->start_time * this->get_time_base();
}

int StreamDecoder::get_stream_index() const noexcept {
  return this->stream->index;
}

enum AVMediaType StreamDecoder::get_media_type() const noexcept {
  return this->media_type;
}

double StreamDecoder::get_time_base() const noexcept {
  return av_q2d(this->stream->time_base);
}

AVCodecContext* StreamDecoder::get_codec_context() const noexcept {
  return this->codec_context;
}

void StreamDecoder::reset() noexcept {
  avcodec_flush_buffers(this->codec_context);

  while (!this->packet_queue.empty()) {
    AVPacket* packet = this->packet_queue.front();
    this->packet_queue.pop_front();
    av_packet_free(&packet);
  }
}

std::vector<AVFrame*> StreamDecoder::decode_next() {
  std::vector<AVFrame*> dec_frames;
  static constexpr int ALLOWED_FAILURES = 5;

  bool decoding_error_thrown = true; //init to true so loop runs
  for (int i = 0; i <= ALLOWED_FAILURES && decoding_error_thrown; i++) {
    decoding_error_thrown = false;

    try {
      dec_frames = decode_packet_queue(this->codec_context, this->packet_queue, this->media_type);
    } catch (ffmpeg_error const& e) {
      decoding_error_thrown = true;
      if (i >= ALLOWED_FAILURES) {
        throw std::runtime_error(fmt::format("[{}] Could not decode next {} "
        "packet: {}", FUNCDINFO, av_get_media_type_string(this->media_type), e.what()));
      }
    }

  }
  
  return dec_frames;
}

StreamDecoder::~StreamDecoder() {
  while (!this->packet_queue.empty()) {
    AVPacket* packet = this->packet_queue.front();
    this->packet_queue.pop_front();
    av_packet_free(&packet);
  }

  if (this->codec_context != nullptr) {
    avcodec_free_context(&(this->codec_context));
  }
};

bool StreamDecoder::has_packets() {
  return !this->packet_queue.empty();
}

void StreamDecoder::push_back(AVPacket* packet) {
  this->packet_queue.push_back(packet);
}
