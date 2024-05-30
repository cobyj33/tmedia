#include <tmedia/ffmpeg/streamdecoder.h>


#include <tmedia/ffmpeg/decode.h>
#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/ffmpeg/avguard.h>
#include <tmedia/util/defines.h>

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

void StreamDecoder::reset() noexcept {
  avcodec_flush_buffers(this->codec_context);

  while (!this->packet_queue.empty()) {
    AVPacket* packet = this->packet_queue.front();
    this->packet_queue.pop_front();
    av_packet_free(&packet);
  }
}

void StreamDecoder::decode_next(std::vector<AVFrame*>& frame_buffer) {
  static constexpr int ALLOWED_FAILURES = 5;

  bool decoding_error_thrown = true; //init to true so loop runs
  for (int i = 0; i <= ALLOWED_FAILURES && decoding_error_thrown; i++) {
    decoding_error_thrown = false;

    try {
      return decode_packet_queue(this->codec_context, this->packet_queue, this->media_type, frame_buffer);
    } catch (ffmpeg_error const& e) {
      decoding_error_thrown = true;
      if (i >= ALLOWED_FAILURES) {
        throw std::runtime_error(fmt::format("[{}] Could not decode next {} "
        "packet: \n\t{}", FUNCDINFO, av_get_media_type_string(this->media_type),
         e.what()));
      }
    }
  }
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


