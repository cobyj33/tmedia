#include "decode.h"
#include <boiler.h>
#include <stdio.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include <libavutil/fifo.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/samplefmt.h>
#include <libavfilter/avfilter.h>

AVFrame** resample_audio_frames(AudioResampler* resampler, AVFrame** originals, int nb_frames) {
    int currently_allocated = 0;
    AVFrame** resampled_frames = (AVFrame**)malloc(sizeof(AVFrame*) * nb_frames);

    for (int i = 0; i < nb_frames; i++) {
        AVFrame* resampledFrame = resample_audio_frame(resampler, originals[i]);
        if (resampledFrame == NULL) {
            free_frame_list(resampled_frames, currently_allocated);
            return NULL;
        }
        resampled_frames[i] = resampledFrame;
        currently_allocated++;
    }

    return resampled_frames;
}

AVFormatContext* open_format_context(const char* fileName, int* result) {
    AVFormatContext* formatContext = NULL;
    *result = avformat_open_input(&formatContext, fileName, NULL, NULL);
    if (*result < 0) {
        avformat_free_context(formatContext);
        return NULL;
    }

    *result = avformat_find_stream_info(formatContext, NULL);
    if (*result < 0) {
        avformat_free_context(formatContext);
        return NULL;
    }

    return formatContext;
}

StreamData** alloc_stream_datas(AVFormatContext* formatContext, const enum AVMediaType* mediaTypes, int nb_target_streams, int* out_stream_count) {
    *out_stream_count = 0;
    StreamData** data = (StreamData**)malloc(sizeof(StreamData*) * nb_target_streams);
    
    int result;
    int skipped = 0;
    for (int i = 0; i < nb_target_streams; i++) {
        StreamData* currentData = (StreamData*)malloc(sizeof(StreamData));
        currentData->codecContext = NULL;
        const AVCodec* decoder;
        int streamIndex = -1;
        streamIndex = av_find_best_stream(formatContext, mediaTypes[i], -1, -1, &decoder, 0);
        if (streamIndex < 0) {
            stream_data_free(currentData);
            skipped++;
            continue;
        }

        currentData->decoder = decoder;
        currentData->stream = formatContext->streams[streamIndex];
        currentData->codecContext = avcodec_alloc_context3(decoder);
        currentData->mediaType = mediaTypes[i];

        result = avcodec_parameters_to_context(currentData->codecContext, currentData->stream->codecpar);
        if (result < 0) {
            stream_data_free(currentData);
            skipped++;
            continue;
        }

        result = avcodec_open2(currentData->codecContext, currentData->decoder, NULL);
        if (result < 0) {
            stream_data_free(currentData);
            skipped++;
            continue;
        }

        data[i - skipped] = currentData;
        *out_stream_count += 1;
    }

    return data;
}

void stream_datas_free(StreamData** streamDatas, int nb_streams) {
    for (int i = 0; i < nb_streams; i++) {
        stream_data_free(streamDatas[i]);
    }
    free(streamDatas);
}

void stream_data_free (StreamData *streamData) {
    if (streamData->codecContext != NULL) {
        avcodec_free_context(&(streamData->codecContext));
    }

    free(streamData);
}

VideoConverter* get_video_converter(int dst_width, int dst_height, enum AVPixelFormat dst_pix_fmt, int src_width, int src_height, enum AVPixelFormat src_pix_fmt) {
    VideoConverter* converter = (VideoConverter*)malloc(sizeof(VideoConverter));
    converter->context = sws_getContext(
            src_width, src_height, src_pix_fmt, 
            dst_width, dst_height, dst_pix_fmt, 
            SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (converter->context == NULL) {
        return NULL;
    }
    
    converter->dst_width = dst_width;
    converter->dst_height = dst_height;
    converter->dst_pix_fmt = dst_pix_fmt;
    converter->src_width = src_width;
    converter->src_height = src_height;
    converter->src_pix_fmt = src_pix_fmt;
    return converter;
}

AudioResampler* get_audio_resampler(int* result, AVChannelLayout* dst_ch_layout, enum AVSampleFormat dst_sample_fmt, int dst_sample_rate, AVChannelLayout* src_ch_layout, enum AVSampleFormat src_sample_fmt, int src_sample_rate) {
    SwrContext* context = swr_alloc();
    *result = swr_alloc_set_opts2(
        &context, dst_ch_layout, dst_sample_fmt, 
        dst_sample_rate, src_ch_layout, src_sample_fmt,
        src_sample_rate, 0, NULL);

    if (*result < 0) {
        if (context == NULL) {
            swr_free(&context);
        }
        return NULL;
    } else {
        *result = swr_init(context);
        if (*result < 0) {
            if (context == NULL) {
                swr_free(&context);
            }
            return NULL;
        }
    }

    AudioResampler* resampler = (AudioResampler*)malloc(sizeof(AudioResampler));
    resampler->context = context;
    resampler->src_sample_rate = src_sample_rate;
    resampler->src_sample_fmt = src_sample_fmt;
    resampler->src_ch_layout = src_ch_layout;
    resampler->dst_sample_rate = dst_sample_rate;
    resampler->dst_sample_fmt = dst_sample_fmt;
    resampler->dst_ch_layout = dst_ch_layout;

    return resampler;
}


void free_video_converter(VideoConverter *converter) {
    sws_freeContext(converter->context);
    free(converter);
}

void free_audio_resampler(AudioResampler *resampler) {
    swr_free(&(resampler->context));
    free(resampler);
}

AVFrame* resample_audio_frame(AudioResampler* resampler, AVFrame* original) {
        int result;
        AVFrame* resampledFrame = av_frame_alloc();
        resampledFrame->sample_rate = resampler->dst_sample_rate;
        resampledFrame->ch_layout = *(resampler->dst_ch_layout);
        resampledFrame->format = resampler->dst_sample_fmt;
        resampledFrame->pts = original->pts;
        resampledFrame->duration = original->duration;

        result = swr_convert_frame(resampler->context, resampledFrame, original);
        if (result < 0) {
            fprintf(stderr, "%s\n", "Unable to resample audio frame");
            av_frame_free(&resampledFrame);
            return NULL;
        }

        return resampledFrame;
}


AVFrame* convert_video_frame(VideoConverter* converter, AVFrame* original) {
    AVFrame* resizedVideoFrame = av_frame_alloc();
    resizedVideoFrame->format = converter->dst_pix_fmt;
    resizedVideoFrame->width = converter->dst_width;
    resizedVideoFrame->height = converter->dst_height;
    resizedVideoFrame->pts = original->pts;
    resizedVideoFrame->repeat_pict = original->repeat_pict;
    resizedVideoFrame->duration = original->duration;
    av_frame_get_buffer(resizedVideoFrame, 1); //watch this alignment
    sws_scale(converter->context, (uint8_t const * const *)original->data, original->linesize, 0, original->height, resizedVideoFrame->data, resizedVideoFrame->linesize);

    return resizedVideoFrame;
}
