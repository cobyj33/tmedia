#include "streamdecoder.h"


#include "decode.h"
#include "boiler.h"
#include "ffmpeg_error.h"
#include "avguard.h"

#include <map>
#include <vector>
#include <memory>
#include <stdexcept>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

StreamDecoder::StreamDecoder(AVFormatContext* format_context, enum AVMediaType media_type) {
  #if AV_FIND_BEST_STREAM_CONST_DECODER
  const AVCodec* decoder;
  #else
  AVCodec* decoder;
  #endif
  
  int stream_index = -1;
  stream_index = av_find_best_stream(format_context, media_type, -1, -1, &decoder, 0);
  if (stream_index < 0) {
    throw std::runtime_error("[StreamDecoder::StreamDecoder] Cannot find media type for type " + std::string(av_get_media_type_string(media_type)));
  }

  this->decoder = decoder;
  this->stream = format_context->streams[stream_index];
  this->codec_context = avcodec_alloc_context3(this->decoder);

  if (this->codec_context == nullptr) {
    throw std::runtime_error("[StreamDecoder::StreamDecoder] Could not create codec context from decoder: " + std::string(decoder->long_name));
  }

  this->media_type = media_type;

  int result = avcodec_parameters_to_context(this->codec_context, this->stream->codecpar);
  if (result < 0) {
    throw std::runtime_error("[StreamDecoder::StreamDecoder] Could not move AVCodec parameters into context: AVERROR error code " + av_strerror_string(result));
  }

  result = avcodec_open2(this->codec_context, this->decoder, NULL);
  if (result < 0) {
    throw std::runtime_error("[StreamDecoder::StreamDecoder] Could not initialize AVCodecContext with AVCodec decoder: AVERROR error code " + av_strerror_string(result));
  }
};

double StreamDecoder::get_average_frame_rate_sec() const {
  return av_q2d(this->stream->avg_frame_rate);
}

double StreamDecoder::get_average_frame_time_sec() const {
  return 1 / av_q2d(this->stream->avg_frame_rate);
}

double StreamDecoder::get_start_time() const {
  if (this->stream->start_time == AV_NOPTS_VALUE)
    return 0.0;
  
  return this->stream->start_time * this->get_time_base();
}

int StreamDecoder::get_stream_index() const {
  return this->stream->index;
}

enum AVMediaType StreamDecoder::get_media_type() const {
  return this->media_type;
}

double StreamDecoder::get_time_base() const {
  return av_q2d(this->stream->time_base);
}

AVCodecContext* StreamDecoder::get_codec_context() const {
  return this->codec_context;
}

void StreamDecoder::reset() {
  avcodec_flush_buffers(this->codec_context);

  while (!this->packet_queue.empty()) {
    AVPacket* packet = this->packet_queue.front();
    this->packet_queue.pop_front();
    av_packet_free(&packet);
  }
}

std::vector<AVFrame*> StreamDecoder::decode_next() {
  std::vector<AVFrame*> decoded_frames;
  static constexpr int ALLOWED_FAILURES = 5;

  bool decoding_error_thrown = true; //init to true so loop runs
  for (int i = 0; i <= ALLOWED_FAILURES && decoding_error_thrown; i++) {
    decoding_error_thrown = false;

    try {
      decoded_frames = decode_packet_queue(this->codec_context, this->packet_queue, this->media_type);
    } catch (ffmpeg_error const& e) {
      decoding_error_thrown = true;
      if (i >= ALLOWED_FAILURES) {
        throw std::runtime_error("[StreamDecoder::decode_next] Could not decode next " + 
        std::string(av_get_media_type_string(this->media_type)) + " packet: " + std::string(e.what()));
      }
    }

  }
  
  return decoded_frames;
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

void StreamDecoder::push_back_packet(AVPacket* packet) {
  this->packet_queue.push_back(packet);
}
