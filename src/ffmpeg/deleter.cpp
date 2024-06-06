#include <tmedia/ffmpeg/deleter.h>

#include <tmedia/util/defines.h> // for unlikely()
#include <new> // for std::bad_alloc

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
}

AVPacket* av_packet_allocx() {
  AVPacket* packet = av_packet_alloc();
  if (unlikely(packet == nullptr)) throw std::bad_alloc();
  return packet;
}

AVFrame* av_frame_allocx() {
  AVFrame* frame = av_frame_alloc();
  if (unlikely(frame == nullptr)) throw std::bad_alloc();
  return frame;
}

void AVFormatContextDeleter::operator()(AVFormatContext* ptr) const {
  if (ptr != nullptr)
    avformat_close_input(&ptr);
}

void AVFrameDeleter::operator()(AVFrame* ptr) const {
  if (ptr != nullptr)
    av_frame_free(&ptr);
}

void AVPacketDeleter::operator()(AVPacket* ptr) const {
  if (ptr != nullptr)
    av_packet_free(&ptr);
}

void AVCodecContextDeleter::operator()(AVCodecContext* ptr) const {
  if (ptr != nullptr)
    avcodec_free_context(&ptr);
}
