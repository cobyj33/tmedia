#include <tmedia/ffmpeg/videoconverter.h>

#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/ffmpeg/deleter.h>
#include <tmedia/ffmpeg/avguard.h>
#include <tmedia/util/defines.h>

#include <stdexcept>
#include <memory>
#include <cassert>

#include <fmt/format.h>

extern "C" {
  #include <libavutil/frame.h>
  #include <libavutil/imgutils.h>
  #include <libswscale/swscale.h>
  #include <libavutil/version.h>
}

VideoConverter::VideoConverter(int dst_width, int dst_height, enum AVPixelFormat dst_pix_fmt, int src_width, int src_height, enum AVPixelFormat src_pix_fmt) {
  /**
   * Note that there were bugs occasionally of src_width and src_height actually
   * returning as negative or 0 from allocated AVFrames and AV_PIX_FMT_NONE
   * resulting as well,
   * so that's why all of these checks exist here. When these edge cases
   * happened as well, FFmpeg aborted with assert(), which meant that I could
   * do nothing except put enforced runtime checks on the constructor
   * of VideoConverter.
   *
   * (This class is only actually ever used a grand total of once in tmedia
   * though, so it should be fine)
  */

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

bool VideoConverter::reset_dst_size(int dst_width, int dst_height) {
  if (dst_width == this->m_dst_width && dst_height == this->m_dst_height)
    return false;

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
  return true;
}

VideoConverter::~VideoConverter() {
  sws_freeContext(this->m_context);
}

void VideoConverter::configure_frame(AVFrame* dest) {
  assert(dest != nullptr);
  av_frame_unref(dest);
  dest->format = this->m_dst_pix_fmt;
  dest->width = this->m_dst_width;
  dest->height = this->m_dst_height;
  
  int err = av_frame_get_buffer(dest, 0);
  if (unlikely(err)) {
    av_frame_unref(dest);
    throw ffmpeg_error(fmt::format("[{}] Failure on "
    "allocating buffers for resized video frame", FUNCDINFO), err);
  }
}

void VideoConverter::convert_video_frame(AVFrame* dest, AVFrame* src) {
  assert(dest != nullptr);
  assert(dest->format == this->m_dst_pix_fmt);
  assert(dest->width == this->m_dst_width);
  assert(dest->height == this->m_dst_height);

  dest->pts = src->pts;
  dest->repeat_pict = src->repeat_pict;
  #if HAS_AVFRAME_DURATION
  dest->duration = src->duration;
  #endif

  (void)sws_scale(this->m_context,
    static_cast<uint8_t const * const *>(src->data), src->linesize, 0,
    src->height, dest->data, dest->linesize);
}
