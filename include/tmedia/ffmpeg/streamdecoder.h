#ifndef TMEDIA_STREAM_DECODER_H
#define TMEDIA_STREAM_DECODER_H

#include <vector>
#include <deque>
#include <mutex>
#include <tmedia/util/defines.h>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

/**
 * StreamDecoder largely wraps around easy decoding functions defined in
 * tmedia/ffmpeg/decode.h
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
 * MediaDecoder class described in tmedia/media/mediadecoder.h
*/
class StreamDecoder {
  private:
    enum AVMediaType media_type;
    AVStream* stream;
    const AVCodec* decoder;
    AVCodecContext* codec_context;
    std::deque<AVPacket*> packet_queue;
    std::mutex queue_mutex; // currently unused

  public:
    StreamDecoder(AVFormatContext* fmt_ctx, enum AVMediaType media_type);
    void reset() noexcept;
    void decode_next(std::vector<AVFrame*>& frame_buffer);

    TMEDIA_ALWAYS_INLINE inline double get_average_frame_rate_sec() const noexcept {
      return av_q2d(this->stream->avg_frame_rate);
    }

    TMEDIA_ALWAYS_INLINE inline double get_avgfts() const noexcept {
      return 1 / av_q2d(this->stream->avg_frame_rate);
    }

    TMEDIA_ALWAYS_INLINE inline double get_start_time() const noexcept {
      return (this->stream->start_time != AV_NOPTS_VALUE) * (this->stream->start_time * this->get_time_base());
    }

    TMEDIA_ALWAYS_INLINE inline int get_stream_index() const noexcept {
      return this->stream->index;
    }

    TMEDIA_ALWAYS_INLINE inline enum AVMediaType get_media_type() const noexcept {
      return this->media_type;
    }

    TMEDIA_ALWAYS_INLINE inline double get_time_base() const noexcept {
      return av_q2d(this->stream->time_base);
    }

    TMEDIA_ALWAYS_INLINE inline AVCodecContext* get_codec_context() const noexcept {
      return this->codec_context;
    }
    
    TMEDIA_ALWAYS_INLINE inline bool has_packets() const {
      return !this->packet_queue.empty();
    }

    TMEDIA_ALWAYS_INLINE inline void push_back(AVPacket* packet) {
      this->packet_queue.push_back(packet);
    }

    ~StreamDecoder();
};

#endif