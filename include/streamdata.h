#ifndef ASCII_VIDEO_STREAM_DATA_INCLUDE
#define ASCII_VIDEO_STREAM_DATA_INCLUDE

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <deque>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

class StreamData {
    public:
        AVFormatContext* format_context;
        enum AVMediaType media_type;
        AVStream* stream;
        const AVCodec* decoder;
        AVCodecContext* codec_context;

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

std::unique_ptr<std::vector<std::unique_ptr<StreamData>>> get_stream_datas(AVFormatContext* format_context, std::vector<enum AVMediaType>& media_types);
StreamData& get_av_media_stream(std::unique_ptr<std::vector<std::unique_ptr<StreamData>>>& stream_datas, enum AVMediaType media_type);
bool has_av_media_stream(std::unique_ptr<std::vector<std::unique_ptr<StreamData>>>& stream_datas, enum AVMediaType media_type);

#endif