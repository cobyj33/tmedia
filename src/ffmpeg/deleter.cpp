#include <tmedia/ffmpeg/deleter.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
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