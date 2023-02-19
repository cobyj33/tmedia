#include "videoconverter.h"
#include "except.h"

extern "C" {
    #include <libavutil/frame.h>
    #include <libswscale/swscale.h>
}

VideoConverter::VideoConverter(int dst_width, int dst_height, enum AVPixelFormat dst_pix_fmt, int src_width, int src_height, enum AVPixelFormat src_pix_fmt) {
    this->m_context = sws_getContext(
            src_width, src_height, src_pix_fmt, 
            dst_width, dst_height, dst_pix_fmt, 
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    if (this->m_context == nullptr) {
        throw ascii::bad_alloc("Allocation of internal SwsContext of AudioResampler failed. Aborting...");
    }
    
    this->m_dst_width = dst_width;
    this->m_dst_height = dst_height;
    this->m_dst_pix_fmt = dst_pix_fmt;
    this->m_src_width = src_width;
    this->m_src_height = src_height;
    this->m_src_pix_fmt = src_pix_fmt;
}

VideoConverter::~VideoConverter() {
    sws_freeContext(this->m_context);
}

AVFrame* VideoConverter::convert_video_frame(AVFrame* original) {
    AVFrame* resized_video_frame = av_frame_alloc();
    if (resized_video_frame == nullptr) {
        throw ascii::bad_alloc("Could not allocate resized frame for VideoConverter");
    }

    resized_video_frame->format = this->m_dst_pix_fmt;
    resized_video_frame->width = this->m_dst_width;
    resized_video_frame->height = this->m_dst_height;
    resized_video_frame->pts = original->pts;
    resized_video_frame->repeat_pict = original->repeat_pict;
    resized_video_frame->duration = original->duration;
    av_frame_get_buffer(resized_video_frame, 1); //watch this alignment
    sws_scale(this->m_context, (uint8_t const * const *)original->data, original->linesize, 0, original->height, resized_video_frame->data, resized_video_frame->linesize);

    return resized_video_frame;
}

int VideoConverter::get_src_width() {
    return this->m_src_width;
};

int VideoConverter::get_src_height() {
    return this->m_src_height;
};

int VideoConverter::get_dst_width() {
    return this->m_dst_width;
};

int VideoConverter::get_dst_height() {
    return this->m_dst_height;
};

enum AVPixelFormat VideoConverter::get_src_pix_fmt() {
    return this->m_src_pix_fmt;
}

enum AVPixelFormat VideoConverter::get_dst_pix_fmt() {
    return this->m_dst_pix_fmt;
}