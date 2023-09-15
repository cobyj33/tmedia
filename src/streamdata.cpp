#include <map>
#include <memory>
#include <stdexcept>

#include "decode.h"
#include "streamdata.h"
#include "except.h"
#include "avguard.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

StreamData::StreamData(AVFormatContext* format_context, enum AVMediaType media_type) {
  #if AV_FIND_BEST_STREAM_CONST_DECODER
  const AVCodec* decoder;
  #else
  AVCodec* decoder;
  #endif
  
  int stream_index = -1;
  stream_index = av_find_best_stream(format_context, media_type, -1, -1, &decoder, 0);
  if (stream_index < 0) {
    throw std::invalid_argument("Cannot find media type for type " + std::string(av_get_media_type_string(media_type)));
  }

  this->decoder = decoder;
  this->stream = format_context->streams[stream_index];
  this->codec_context = avcodec_alloc_context3(this->decoder);

  if (this->codec_context == nullptr) {
    throw std::invalid_argument("Could not create codec context from decoder: " + std::string(decoder->long_name));
  }

  this->media_type = media_type;

  int result = avcodec_parameters_to_context(this->codec_context, this->stream->codecpar);
  if (result < 0) {
    throw std::invalid_argument("Could not move AVCodec parameters into context: AVERROR error code " + std::to_string(AVERROR(result)));
  }

  result = avcodec_open2(this->codec_context, this->decoder, NULL);
  if (result < 0) {
    throw std::invalid_argument("Could not initialize AVCodecContext with AVCodec decoder: AVERROR error code " + std::to_string(AVERROR(result)));
  }
};

double StreamData::get_average_frame_rate_sec() const {
  return av_q2d(this->stream->avg_frame_rate);
}

double StreamData::get_average_frame_time_sec() const {
  return 1 / av_q2d(this->stream->avg_frame_rate);
}

double StreamData::get_start_time() const {
  return this->stream->start_time * this->get_time_base();
}

int StreamData::get_stream_index() const {
  return this->stream->index;
}

void StreamData::flush() {
  AVCodecContext* codec_context = this->get_codec_context();
  avcodec_flush_buffers(codec_context);
}

enum AVMediaType StreamData::get_media_type() const {
  return this->media_type;
}

double StreamData::get_time_base() const {
  return av_q2d(this->stream->time_base);
}

AVCodecContext* StreamData::get_codec_context() const {
  return this->codec_context;
}

void StreamData::clear_queue() {
  while (!this->packet_queue.empty()) {
    AVPacket* packet = this->packet_queue.front();
    this->packet_queue.pop_front();
    av_packet_free(&packet);
  }
}

std::vector<AVFrame*> StreamData::decode_next() {
  if (this->media_type == AVMEDIA_TYPE_VIDEO) {
    std::vector<AVFrame*> decoded_frames = decode_video_packet_queue(this->codec_context, this->packet_queue);
    return decoded_frames;
  } else if (this->media_type == AVMEDIA_TYPE_AUDIO) {
    std::vector<AVFrame*> decoded_frames = decode_audio_packet_queue(this->codec_context, this->packet_queue);
    return decoded_frames;
  }

  throw std::runtime_error("[StreamData::decode_next] Could not decode next video packet, Media type " + std::string(av_get_media_type_string(this->media_type)) + " is currently unsupported");
}

StreamData::~StreamData() {
  this->clear_queue();
  if (this->codec_context != nullptr) {
    avcodec_free_context(&(this->codec_context));
  }
};

std::map<enum AVMediaType, std::shared_ptr<StreamData>> get_stream_datas(AVFormatContext* format_context) {
  const int NUM_OF_MEDIA_TYPES_TO_SEARCH = 2;
  enum AVMediaType MEDIA_TYPES_TO_SEARCH[NUM_OF_MEDIA_TYPES_TO_SEARCH] = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
  std::map<enum AVMediaType, std::shared_ptr<StreamData>> stream_datas;

  for (int i = 0; i < NUM_OF_MEDIA_TYPES_TO_SEARCH; i++) {
    try {
      std::shared_ptr<StreamData> stream_data_ptr = std::make_shared<StreamData>(format_context, MEDIA_TYPES_TO_SEARCH[i]);
      stream_datas[MEDIA_TYPES_TO_SEARCH[i]] = stream_data_ptr;
    } catch (std::invalid_argument const& e) {
      continue;
    }
  }

  return stream_datas;
}