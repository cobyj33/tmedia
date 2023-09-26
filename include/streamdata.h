#ifndef ASCII_VIDEO_STREAM_DATA_INCLUDE
#define ASCII_VIDEO_STREAM_DATA_INCLUDE

#include <stdexcept>
#include <string>
#include <map>
#include <memory>
#include <deque>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

class StreamData {
  private:
    enum AVMediaType media_type;
    AVStream* stream;
    const AVCodec* decoder;
    AVCodecContext* codec_context;
    
  public:

    std::deque<AVPacket*> packet_queue;

    StreamData(AVFormatContext* format_context, enum AVMediaType media_type);

    double get_time_base() const;
    double get_average_frame_rate_sec() const;
    double get_average_frame_time_sec() const;
    double get_start_time() const;
    int get_stream_index() const;
    enum AVMediaType get_media_type() const;
    AVCodecContext* get_codec_context() const;

    std::vector<AVFrame*> decode_next();
    void flush();
    void clear_queue();

    ~StreamData();
};

std::map<enum AVMediaType, std::shared_ptr<StreamData>> get_stream_datas(AVFormatContext* format_context);

#endif