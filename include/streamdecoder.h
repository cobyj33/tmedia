#ifndef ASCII_VIDEO_STREAM_DECODER_INCLUDE
#define ASCII_VIDEO_STREAM_DECODER_INCLUDE

#include <stdexcept>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <deque>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

class StreamDecoder {
  private:
    enum AVMediaType media_type;
    AVStream* stream;
    const AVCodec* decoder;
    AVCodecContext* codec_context;
    std::deque<AVPacket*> packet_queue;

  public:
    StreamDecoder(AVFormatContext* format_context, enum AVMediaType media_type);

    double get_time_base() const;
    double get_average_frame_rate_sec() const;
    double get_average_frame_time_sec() const;
    double get_start_time() const;
    int get_stream_index() const;
    enum AVMediaType get_media_type() const;
    AVCodecContext* get_codec_context() const;
    
    void reset();

    bool has_packets();
    void push_back_packet(AVPacket*);

    std::vector<AVFrame*> decode_next();

    ~StreamDecoder();
};

#endif