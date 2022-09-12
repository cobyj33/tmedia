#include "decode.h"
#include <cstdlib>
#include <iostream>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <vector>
extern "C" {
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

decoder_function get_stream_decoder(AVMediaType mediaType) {
    switch (mediaType) {
        case AVMEDIA_TYPE_VIDEO: return decode_video_packet;
        case AVMEDIA_TYPE_AUDIO: return decode_audio_packet;
        default: 
                std::cout << "CANNOT RETURN DECODER FOR MEDIA TYPE: " << mediaType << std::endl;
                return nullptr;
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
            std::cout << "Video Packet reading Error: " << result << std::endl;
            free(videoFrames);
            return nullptr;
        } else if (*result != AVERROR(EAGAIN) ) {
            std::cout << "Video Packet reading Error: " << result << std::endl;
            free(videoFrames);
            return nullptr;
        }
    }

    AVFrame* videoFrame = av_frame_alloc();
    *result = avcodec_receive_frame(videoCodecContext, videoFrame);
    if (*result == AVERROR(EAGAIN)) {
        free(videoFrames);
        av_frame_free(&videoFrame);
        return nullptr;
    } else if (*result < 0) {
        std::cerr << "FATAL ERROR WHILE READING VIDEO PACKET: " << *result << std::endl;
        free(videoFrames);
        av_frame_free(&videoFrame);
        return nullptr;
    } else {
        while (*result == 0) {
            AVFrame* savedFrame = av_frame_alloc();
            *result = av_frame_ref(savedFrame, videoFrame);
            if (*result < 0) {
                std::cerr << "ERROR WHILE REFERENCING VIDEO FRAMES DURING DECODING" << std::endl;
                free_frame_list(videoFrames, *nb_out);
                *nb_out = 0;
                av_frame_free(&savedFrame);
                av_frame_free(&videoFrame);
                return nullptr;
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
}




AVFrame** decode_audio_packet(AVCodecContext* audioCodecContext, AVPacket* audioPacket, int* result, int* nb_out) {
    *nb_out = 0;
    *result = 0;
    int reservedFrames = 5;
    AVFrame** audioFrames = (AVFrame**)malloc(sizeof(AVFrame*) * reservedFrames);

    *result = avcodec_send_packet(audioCodecContext, audioPacket);
    if (*result < 0) {
        std::cerr << "ERROR WHILE SENDING AUDIO PACKET: " << result << std::endl; 
        free(audioFrames);
        return nullptr;
    }

    AVFrame* audioFrame = av_frame_alloc();
    *result = avcodec_receive_frame(audioCodecContext, audioFrame);
    if (*result < 0) {
        char error_buffer[512];
        av_strerror(*result, error_buffer, 512);
        std::cerr << "ERROR WHILE RECEIVING AUDIO FRAMES: " << error_buffer << std::endl; 
        free(audioFrames);
        av_frame_free(&audioFrame);
        return nullptr;
    }

    while (*result == 0) {
        AVFrame* savedFrame = av_frame_alloc();
        *result = av_frame_ref(savedFrame, audioFrame); 
        if (*result < 0) {
            std::cerr << "ERROR WHILE REFERENCING AUDIO FRAMES DURING DECODING" << std::endl;
            free_frame_list(audioFrames, *nb_out);
            *nb_out = 0;
            av_frame_free(&savedFrame);
            av_frame_free(&audioFrame);
            return nullptr;
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

    av_frame_free(&audioFrame);
    return audioFrames;
}

