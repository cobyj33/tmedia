#include <tmedia/ffmpeg/videoconverter.h>

#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/ffmpeg/deleter.h>
#include <tmedia/ffmpeg/avguard.h>
#include <tmedia/util/defines.h>

#include <stdexcept>
#include <memory>

#include <fmt/format.h>

extern "C" {
  #include <libavutil/frame.h>
  #include <libswscale/swscale.h>
  #include <libavutil/version.h>
}

VideoConverter::VideoConverter(int dst_width, int dst_height, enum AVPixelFormat dst_pix_fmt, int src_width, int src_height, enum AVPixelFormat src_pix_fmt) {
  if (dst_width <= 0 || dst_height <= 0) {
    throw std::runtime_error(fmt::format("[{}] Video Converter must have "
    "non-zero destination dimensions: (got width of {} and height of {} )",
    FUNCDINFO, dst_width, dst_height));
  }

  if (src_width <= 0 || src_height <= 0) {
    throw std::runtime_error(fmt::format("[{}] Video Converter must have "
    "non-zero source dimensions: (got width of {} and height of {})",
    FUNCDINFO, src_width, src_height));
  }

  if (src_pix_fmt == AV_PIX_FMT_NONE) {
    throw std::runtime_error(fmt::format("[{}] Video Converter must have "
    "a defined source pixel format: got AV_PIX_FMT_NONE", FUNCDINFO));
  }

  if (dst_pix_fmt == AV_PIX_FMT_NONE) {
    throw std::runtime_error(fmt::format("[{}] Video Converter must have a "
    "defined dest pixel format: got AV_PIX_FMT_NONE", FUNCDINFO));
  }

  this->m_context = sws_getContext(
      src_width, src_height, src_pix_fmt, 
      dst_width, dst_height, dst_pix_fmt, 
      SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  if (this->m_context == nullptr) {
    throw std::runtime_error(fmt::format("[{}] Allocation of internal "
    "SwsContext of Video Converter failed", FUNCDINFO));
  }
  
  this->m_dst_width = dst_width;
  this->m_dst_height = dst_height;
  this->m_dst_pix_fmt = dst_pix_fmt;
  this->m_src_width = src_width;
  this->m_src_height = src_height;
  this->m_src_pix_fmt = src_pix_fmt;
}

void VideoConverter::reset_dst_size(int dst_width, int dst_height) {
  if (dst_width == this->m_dst_width && dst_height == this->m_dst_height)
    return;

  SwsContext* new_context = sws_getContext(
      this->m_src_width, this->m_src_height, this->m_src_pix_fmt, 
      dst_width, dst_height, this->m_dst_pix_fmt, 
      SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  if (new_context == nullptr) {
    throw std::runtime_error(fmt::format("[{}] Allocation of internal "
    "SwsContext of Video Converter failed", FUNCDINFO));
  }

  sws_freeContext(this->m_context);
  this->m_context = new_context;

  this->m_dst_width = dst_width;
  this->m_dst_height = dst_height;
}

VideoConverter::~VideoConverter() {
  sws_freeContext(this->m_context);
}

AVFrame* VideoConverter::convert_video_frame(AVFrame* original) {
  AVFrame* resized_video_frame = av_frame_allocx();

  resized_video_frame->format = this->m_dst_pix_fmt;
  resized_video_frame->width = this->m_dst_width;
  resized_video_frame->height = this->m_dst_height;
  resized_video_frame->pts = original->pts;
  resized_video_frame->repeat_pict = original->repeat_pict;
  #if HAS_AVFRAME_DURATION
  resized_video_frame->duration = original->duration;
  #endif
  
  int err = av_frame_get_buffer(resized_video_frame, 1); //watch this alignment
  if (unlikely(err)) {
    av_frame_free(&resized_video_frame);
    throw ffmpeg_error(fmt::format("[{}] Failure on "
    "allocating buffers for resized video frame", FUNCDINFO), err);
  }

  (void)sws_scale(this->m_context,
    (uint8_t const * const *)original->data, original->linesize, 0,
    original->height, resized_video_frame->data, resized_video_frame->linesize);

  return resized_video_frame;
}