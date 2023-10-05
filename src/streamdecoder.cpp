#include <map>
#include <memory>
#include <stdexcept>

#include "decode.h"
#include "boiler.h"
#include "streamdecoder.h"
#include "except.h"
#include "avguard.h"

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

  try {
    decoded_frames = std::move(decode_packet_queue(this->codec_context, this->packet_queue, this->media_type));
  } catch (std::runtime_error const& e) {
    throw std::runtime_error("[StreamDecoder::decode_next] Could not decode next "
    "video packet: " + std::string(e.what()));
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

std::map<enum AVMediaType, std::shared_ptr<StreamDecoder>> get_stream_decoders(AVFormatContext* format_context) {
  const int NUM_OF_MEDIA_TYPES_TO_SEARCH = 2;
  enum AVMediaType MEDIA_TYPES_TO_SEARCH[NUM_OF_MEDIA_TYPES_TO_SEARCH] = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
  std::map<enum AVMediaType, std::shared_ptr<StreamDecoder>> stream_decoders;

  for (int i = 0; i < NUM_OF_MEDIA_TYPES_TO_SEARCH; i++) {
    try {
      std::shared_ptr<StreamDecoder> stream_decoder_ptr = std::make_shared<StreamDecoder>(format_context, MEDIA_TYPES_TO_SEARCH[i]);
      stream_decoders[MEDIA_TYPES_TO_SEARCH[i]] = stream_decoder_ptr;
    } catch (std::invalid_argument const& e) {
      continue;
    }
  }

  return stream_decoders;
}

