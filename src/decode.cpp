#include "decode.h"
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#include "except.h"

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

void clear_av_frame_list(std::vector<AVFrame*>& frameList) {
    for (int i = 0; i < frameList.size(); i++) {
        av_frame_free(&(frameList[i]));
    }
    frameList.clear();
}


std::vector<AVFrame*> decode_video_packet(AVCodecContext* videoCodecContext, AVPacket* videoPacket) {
    std::vector<AVFrame*> videoFrames;
    int result;

    result = avcodec_send_packet(videoCodecContext, videoPacket);
    if (result < 0) {
        if (result == AVERROR(EAGAIN) ) {
            throw ascii::ffmpeg_error(" EAGAIN error while sending video packet, send more data on consecutive calls ", result);
        } else if (result != AVERROR(EAGAIN) ) {
            throw ascii::ffmpeg_error("Error while sending video packet: ", result);
        }
    }

    AVFrame* videoFrame = av_frame_alloc();
    result = avcodec_receive_frame(videoCodecContext, videoFrame);
    if (result < 0) {
        av_frame_free(&videoFrame);
        if (result != AVERROR(EAGAIN)) {
            throw ascii::ffmpeg_error("FATAL ERROR WHILE RECEIVING VIDEO FRAME:", result);
        }
    }


    while (result == 0) {
        AVFrame* savedFrame = av_frame_alloc();
        result = av_frame_ref(savedFrame, videoFrame);
        if (result < 0) {
            clear_av_frame_list(videoFrames);
            av_frame_free(&savedFrame);
            av_frame_free(&videoFrame);
            throw ascii::ffmpeg_error("ERROR WHILE REFERENCING VIDEO FRAMES DURING DECODING:", result);
        }

        videoFrames.push_back(savedFrame);
        av_frame_unref(videoFrame);
        result = avcodec_receive_frame(videoCodecContext, videoFrame);
    }
    
    av_frame_free(&videoFrame);
    return videoFrames;
}




std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audioCodecContext, AVPacket* audioPacket) {
    std::vector<AVFrame*> audioFrames;
    int result;

    result = avcodec_send_packet(audioCodecContext, audioPacket);
    if (result < 0) {
        throw ascii::ffmpeg_error("ERROR WHILE SENDING AUDIO PACKET ", result);
    }

    AVFrame* audioFrame = av_frame_alloc();
    result = avcodec_receive_frame(audioCodecContext, audioFrame);
    if (result < 0) { // if EAGAIN, caller should catch and send more data
        av_frame_free(&audioFrame);
        throw ascii::ffmpeg_error("FATAL ERROR WHILE RECEIVING AUDIO FRAME ", result);
    }

    while (result == 0) {
        AVFrame* savedFrame = av_frame_alloc();
        result = av_frame_ref(savedFrame, audioFrame); 
        if (result < 0) {
            clear_av_frame_list(audioFrames);
            av_frame_free(&savedFrame);
            av_frame_free(&audioFrame);
            throw ascii::ffmpeg_error("ERROR WHILE REFERENCING AUDIO FRAMES DURING DECODING ", result);
        }

        av_frame_unref(audioFrame);
        audioFrames.push_back(savedFrame);
        result = avcodec_receive_frame(audioCodecContext, audioFrame);
    }

    av_frame_free(&audioFrame);
    return audioFrames;
}

