#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

typedef AVFrame** (*decoder_function)(AVCodecContext*, AVPacket*, int*, int*);
decoder_function get_stream_decoder(AVMediaType mediaType);

void free_frame_list(AVFrame** frameList, int count);
AVFrame** decode_video_packet(AVCodecContext* videoCodecContext, AVPacket* videoPacket, int* result, int* nb_out);
AVFrame** decode_audio_packet(AVCodecContext* audioCodecContext, AVPacket* audioPacket, int* result, int* nb_out);
