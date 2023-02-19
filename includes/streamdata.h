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

        StreamData(AVFormatContext* format_context, enum AVMediaType media_type);

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