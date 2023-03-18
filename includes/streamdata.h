#ifndef ASCII_VIDEO_STREAM_DATA_INCLUDE
#define ASCII_VIDEO_STREAM_DATA_INCLUDE

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

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

        PlayheadList<AVPacket*> packets;

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

        ~StreamData();
};

class StreamDataGroup {
    private:
        AVFormatContext* m_format_context;
        std::vector<std::shared_ptr<StreamData>> m_datas;

    public:
        StreamDataGroup(AVFormatContext* format_context, const enum AVMediaType* media_types, int nb_target_streams);

        int get_nb_streams();
        StreamData& get_av_media_stream(AVMediaType media_type);
        bool has_av_media_stream(AVMediaType media_type);

        StreamData& operator[](int);
};



#endif