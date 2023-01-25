#ifndef ASCII_VIDEO_STREAM_DATA_INCLUDE
#define ASCII_VIDEO_STREAM_DATA_INCLUDE

#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

class StreamData {
    public:
        AVFormatContext* formatContext;
        enum AVMediaType mediaType;
        AVStream* stream;
        const AVCodec* decoder;
        AVCodecContext* codecContext;

        StreamData(AVFormatContext* formatContext, enum AVMediaType mediaType);

        ~StreamData();
};

class StreamDataGroup {
    private:
        AVFormatContext* m_formatContext;
        std::vector<StreamData*> m_datas;

    public:
        StreamDataGroup(AVFormatContext* formatContext, const enum AVMediaType* mediaTypes, int nb_target_streams);

        int get_nb_streams();
        StreamData* get_media_stream(AVMediaType media_type);
        bool has_media_stream(AVMediaType media_type);

        StreamData*& operator[](int);
        ~StreamDataGroup();
};



#endif