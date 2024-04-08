#ifndef TMEDIA_STREAM_DECODER_H
#define TMEDIA_STREAM_DECODER_H

#include <vector>
#include <deque>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

/**
 * StreamDecoder largely wraps around easy decoding functions defined in
 * decode.h
*/

/**
 * An abstraction upon an AVCodecContext and an AVStream which allows for easy
 * decoding of sequential AVFrame data from a given stream
 * 
 * Note that the StreamDecoder's lifetime must not be longer than the
 * AVFormatContext which is used to create it.
 * 
 * Note that a StreamDecoder is not responsible for putting the packets needed
 * for decoding into its own packet queue. This is the responsibility of the
 * MediaDecoder class described in mediadecoder.h
*/
class StreamDecoder {
  private:
    enum AVMediaType media_type;
    AVStream* stream;
    const AVCodec* decoder;
    AVCodecContext* codec_context;
    std::deque<AVPacket*> packet_queue;

  public:
    StreamDecoder(AVFormatContext* fmt_ctx, enum AVMediaType media_type);

    inline double get_average_frame_rate_sec() const noexcept {
      return av_q2d(this->stream->avg_frame_rate);
    }

    inline double get_avgfts() const noexcept {
      return 1 / av_q2d(this->stream->avg_frame_rate);
    }

    inline double get_start_time() const noexcept {
      return (this->stream->start_time != AV_NOPTS_VALUE) * (this->stream->start_time * this->get_time_base());
    }

    inline int get_stream_index() const noexcept {
      return this->stream->index;
    }

    inline enum AVMediaType get_media_type() const noexcept {
      return this->media_type;
    }

    inline double get_time_base() const noexcept {
      return av_q2d(this->stream->time_base);
    }

    inline AVCodecContext* get_codec_context() const noexcept {
      return this->codec_context;
    }
    
    void reset() noexcept;

    bool has_packets();
    void push_back(AVPacket*);

    std::vector<AVFrame*> decode_next();

    ~StreamDecoder();
};

#endif