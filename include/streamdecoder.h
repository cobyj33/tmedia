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

    double get_time_base() const noexcept;
    double get_average_frame_rate_sec() const noexcept;
    double get_avgfts() const noexcept;
    double get_start_time() const noexcept;
    int get_stream_index() const noexcept;
    enum AVMediaType get_media_type() const noexcept;
    AVCodecContext* get_codec_context() const noexcept;
    
    void reset() noexcept;

    bool has_packets();
    void push_back(AVPacket*);

    std::vector<AVFrame*> decode_next();

    ~StreamDecoder();
};

#endif