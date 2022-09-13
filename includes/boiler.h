#pragma once
#include <libavutil/channel_layout.h>
#include <libavutil/pixfmt.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

typedef struct StreamData {
    AVFormatContext* formatContext;
    AVMediaType mediaType;
    AVStream* stream;
    const AVCodec* decoder;
    AVCodecContext* codecContext;
} StreamData;

typedef struct AudioResampler {
    SwrContext* context;
    int src_sample_rate;
    int src_sample_fmt;
    AVChannelLayout* src_ch_layout;
    int dst_sample_rate;
    int dst_sample_fmt;
    AVChannelLayout* dst_ch_layout;
} AudioResampler;

typedef struct VideoConverter {
    SwsContext* context;
    int src_width;
    int src_height;
    AVPixelFormat src_pix_fmt;
    int dst_width;
    int dst_height;
    AVPixelFormat dst_pix_fmt;
} VideoConverter;

void stream_data_free(StreamData* streamData);
void stream_datas_free(StreamData** streamDatas, int nb_streams);

AVFormatContext* open_format_context(const char* fileName, int* result);
StreamData** alloc_stream_datas(AVFormatContext* formatContext, const AVMediaType* mediaTypes, int nb_target_streams, int* out_stream_count);


AudioResampler* get_audio_resampler(int* result, AVChannelLayout* dst_ch_layout, AVSampleFormat dst_sample_fmt, int dst_sample_rate, AVChannelLayout* src_ch_layout, AVSampleFormat src_sample_fmt, int src_sample_rate);
VideoConverter* get_video_converter(int dst_width, int dst_height, AVPixelFormat dst_pix_fmt, int src_width, int src_height, AVPixelFormat src_pix_fmt);
void free_audio_resampler(AudioResampler* resampler);
void free_video_converter(VideoConverter* converter);

AVFrame* convert_video_frame(VideoConverter* converter, AVFrame* original);
AVFrame* resample_audio_frame(AudioResampler* resampler, AVFrame* original);
AVFrame** resample_audio_frames(AudioResampler* resampler, AVFrame** originals, int nb_frames);
