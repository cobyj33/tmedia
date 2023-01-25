
#include "decode.h"
#include <boiler.h>
#include <cstdio>

extern "C" {
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
}


AVFormatContext* open_format_context(const char* fileName, int* result) {
    AVFormatContext* formatContext = NULL;
    *result = avformat_open_input(&formatContext, fileName, NULL, NULL);
    if (*result < 0) {
        if (formatContext == NULL) {
            avformat_free_context(formatContext);
        }
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
    if (data == NULL) {
        return NULL;
    }
    
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




