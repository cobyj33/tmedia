#ifndef ASCII_VIDEO_BOILER
#define ASCII_VIDEO_BOILER

extern "C" {
    #include <libavutil/channel_layout.h>
    #include <libavutil/pixfmt.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswresample/swresample.h>
    #include <libswscale/swscale.h>
}

typedef struct StreamData {
    AVFormatContext* formatContext;
    enum AVMediaType mediaType;
    AVStream* stream;
    const AVCodec* decoder;
    AVCodecContext* codecContext;
} StreamData;


typedef struct VideoConverter {
    struct SwsContext* context;
    int src_width;
    int src_height;
    enum AVPixelFormat src_pix_fmt;
    int dst_width;
    int dst_height;
    enum AVPixelFormat dst_pix_fmt;
} VideoConverter;

void stream_data_free(StreamData* streamData);
void stream_datas_free(StreamData** streamDatas, int nb_streams);

AVFormatContext* open_format_context(const char* fileName, int* result);
StreamData** alloc_stream_datas(AVFormatContext* formatContext, const enum AVMediaType* mediaTypes, int nb_target_streams, int* out_stream_count);


VideoConverter* get_video_converter(int dst_width, int dst_height, enum AVPixelFormat dst_pix_fmt, int src_width, int src_height, enum AVPixelFormat src_pix_fmt);
void free_video_converter(VideoConverter* converter);

AVFrame* convert_video_frame(VideoConverter* converter, AVFrame* original);
#endif
