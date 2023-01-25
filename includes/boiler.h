#ifndef ASCII_VIDEO_BOILER
#define ASCII_VIDEO_BOILER

extern "C" {
    #include <libavutil/channel_layout.h>
    #include <libavutil/pixfmt.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
}

typedef struct StreamData {
    AVFormatContext* formatContext;
    enum AVMediaType mediaType;
    AVStream* stream;
    const AVCodec* decoder;
    AVCodecContext* codecContext;
} StreamData;



void stream_data_free(StreamData* streamData);
void stream_datas_free(StreamData** streamDatas, int nb_streams);

AVFormatContext* open_format_context(const char* fileName, int* result);
StreamData** alloc_stream_datas(AVFormatContext* formatContext, const enum AVMediaType* mediaTypes, int nb_target_streams, int* out_stream_count);

#endif
