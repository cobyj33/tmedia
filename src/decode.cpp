#include "decode.h"
#include <cstdlib>
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

decoder_function get_stream_decoder(enum AVMediaType mediaType) {
    switch (mediaType) {
        case AVMEDIA_TYPE_VIDEO: return decode_video_packet;
        case AVMEDIA_TYPE_AUDIO: return decode_audio_packet;
        default: {
                    fprintf(stderr, "%s %s\n", "CANNOT RETURN DECODER FOR MEDIA TYPE: ", av_get_media_type_string(mediaType));
                    return NULL;
                 }
    }
}

void free_frame_list(AVFrame** frameList, int count) {
    for (int i = 0; i < count; i++) {
        av_frame_free(&(frameList[i]));
    }

    free(frameList);
}


AVFrame** decode_video_packet(AVCodecContext* videoCodecContext, AVPacket* videoPacket, int* result, int* nb_out) {
    *nb_out = 0;
    int reservedFrames = 5;
    AVFrame** videoFrames = (AVFrame**)malloc(sizeof(AVFrame*) * reservedFrames);

    *result = avcodec_send_packet(videoCodecContext, videoPacket);
    if (*result < 0) {
        if (*result == AVERROR(EAGAIN) ) {
            free(videoFrames);
            return NULL;
        } else if (*result != AVERROR(EAGAIN) ) {
            char err[128];
            av_strerror(*result, err, 128);
            fprintf(stderr, "%s %s\n", "ERROR WHILE SENDING VIDEO PACKET:", err);
            free(videoFrames);
            return NULL;
        }
    }

    AVFrame* videoFrame = av_frame_alloc();
    *result = avcodec_receive_frame(videoCodecContext, videoFrame);
    if (*result < 0) {
        if (*result != AVERROR(EAGAIN)) {
            char error_buffer[128];
            av_strerror(*result, error_buffer, 128);
            fprintf(stderr, "%s %s\n", "FATAL ERROR WHILE RECEIVING VIDEO FRAME:", error_buffer);
        }
        free(videoFrames);
        av_frame_free(&videoFrame);
        return NULL;
    }


    while (*result == 0) {
        AVFrame* savedFrame = av_frame_alloc();
        *result = av_frame_ref(savedFrame, videoFrame);
        if (*result < 0) {
            char error_buffer[128];
            av_strerror(*result, error_buffer, 128);
            fprintf(stderr, "%s %s\n", "ERROR WHILE REFERENCING VIDEO FRAMES DURING DECODING:", error_buffer);
            free_frame_list(videoFrames, *nb_out);
            *nb_out = 0;
            av_frame_free(&savedFrame);
            av_frame_free(&videoFrame);
            return NULL;
        }

        videoFrames[*nb_out] = savedFrame;
        *nb_out += 1;
        if (*nb_out >= reservedFrames) {
            reservedFrames *= 2;
            videoFrames = (AVFrame**)realloc(videoFrames, sizeof(AVFrame*) * reservedFrames);
        }

        av_frame_unref(videoFrame);
        *result = avcodec_receive_frame(videoCodecContext, videoFrame);
    }
    *result = 0;
    av_frame_free(&videoFrame);

    return videoFrames;
}




AVFrame** decode_audio_packet(AVCodecContext* audioCodecContext, AVPacket* audioPacket, int* result, int* nb_out) {
    *nb_out = 0;
    *result = 0;
    int reservedFrames = 5;
    AVFrame** audioFrames = (AVFrame**)malloc(sizeof(AVFrame*) * reservedFrames);

    *result = avcodec_send_packet(audioCodecContext, audioPacket);
    if (*result < 0) {
        char err[128];
        av_strerror(*result, err, 128);
        fprintf(stderr, "%s %s\n", "ERROR WHILE SENDING AUDIO PACKET:", err);
        free(audioFrames);
        return NULL;
    }

    AVFrame* audioFrame = av_frame_alloc();
    *result = avcodec_receive_frame(audioCodecContext, audioFrame);
    if (*result < 0) {
        if (*result != AVERROR(EAGAIN)) {
            char error_buffer[128];
            av_strerror(*result, error_buffer, 128);
            fprintf(stderr, "%s %s\n", "FATAL ERROR WHILE RECEIVING AUDIO FRAME:", error_buffer);
        }
        free(audioFrames);
        av_frame_free(&audioFrame);
        return NULL;
    }

    while (*result == 0) {
        AVFrame* savedFrame = av_frame_alloc();
        *result = av_frame_ref(savedFrame, audioFrame); 
        if (*result < 0) {
            char error_buffer[128];
            av_strerror(*result, error_buffer, 128);
            fprintf(stderr, "%s %s\n", "ERROR WHILE REFERENCING AUDIO FRAMES DURING DECODING:", error_buffer);
            free_frame_list(audioFrames, *nb_out);
            *nb_out = 0;
            av_frame_free(&savedFrame);
            av_frame_free(&audioFrame);
            return NULL;
        }

        av_frame_unref(audioFrame);
        if (*nb_out >= reservedFrames) {
            reservedFrames *= 2;
            audioFrames = (AVFrame**)realloc(audioFrames, sizeof(AVFrame*) * reservedFrames);
        }

        audioFrames[*nb_out] = savedFrame;
        (*nb_out)++;
        *result = avcodec_receive_frame(audioCodecContext, audioFrame);
    }

    *result = 0;
    av_frame_free(&audioFrame);
    return audioFrames;
}

