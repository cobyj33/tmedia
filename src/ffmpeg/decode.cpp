#include <tmedia/ffmpeg/decode.h>

#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/ffmpeg/deleter.h>
#include <tmedia/util/defines.h>

#include <fmt/format.h>

#include <memory>
#include <vector>
#include <deque>
#include <stdexcept>
#include <string>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

void clear_avframe_list(std::vector<AVFrame*>& frame_list) {
  for (std::size_t i = 0; i < frame_list.size(); i++) {
    av_frame_free(&(frame_list[i]));
  }
  frame_list.resize(0);
}


void decode_video_packet(AVCodecContext* video_codec_context, AVPacket* video_packet, std::vector<AVFrame*>& frame_buffer) {
  int result = avcodec_send_packet(video_codec_context, video_packet);
  if (result != 0) {
    throw ffmpeg_error(fmt::format("[{}] error while sending video packet: ",
                                    FUNCDINFO), result);
  }

  while (result == 0) {
    // av_frame_alloc used here instead of av_frame_allocx, since if
    // av_frame_alloc fails, we need to clear frame_buffer before throwing
    // an error rather than throwing std::bad_alloc directly
    
    // std::unique_ptr used because video_frame needs to be freed
    // in case frame_buffer.push_back fails for some reason

    std::unique_ptr<AVFrame, AVFrameDeleter> video_frame(av_frame_alloc());
    if (unlikely(video_frame.get() == nullptr)) {
      clear_avframe_list(frame_buffer);
      throw ffmpeg_error("Failed to allocate AVFrame", result);
    }

    result = avcodec_receive_frame(video_codec_context, video_frame.get());
    if (result < 0) {
      if (result == AVERROR(EAGAIN) && !frame_buffer.empty())
        return;
      clear_avframe_list(frame_buffer);
      throw ffmpeg_error(fmt::format("[{}] error while receiving video "
      "frames during decoding: ", FUNCDINFO), result);
    }

    frame_buffer.push_back(video_frame.release());
  }
}




void decode_audio_packet(AVCodecContext* audio_codec_context, AVPacket* audio_packet, std::vector<AVFrame*>& frame_buffer) {
  int result = avcodec_send_packet(audio_codec_context, audio_packet);
  if (result != 0) {
    throw ffmpeg_error(fmt::format("[{}] error while sending audio packet ",
                                    FUNCDINFO), result);
  }

  while (result == 0) {
    // av_frame_alloc used here instead of av_frame_allocx, since if
    // av_frame_alloc fails, we need to clear frame_buffer before throwing
    // an error rather than throwing std::bad_alloc directly
    std::unique_ptr<AVFrame, AVFrameDeleter> audio_frame(av_frame_alloc());
    if (unlikely(audio_frame.get() == nullptr)) {
      clear_avframe_list(frame_buffer);
      throw ffmpeg_error("Failed to allocate AVFrame", result);
    }

    result = avcodec_receive_frame(audio_codec_context, audio_frame.get());
    if (result < 0) {
      if (result == AVERROR(EAGAIN) && !frame_buffer.empty())
        return; // most common return path
      clear_avframe_list(frame_buffer);
      throw ffmpeg_error(fmt::format("[{}] error while receiving audio "
      "frames during decoding: ", FUNCDINFO), result);
    }

    frame_buffer.push_back(audio_frame.release());
  }
}

void decode_packet_queue(AVCodecContext* codec_context, std::deque<AVPacket*>& packet_queue, enum AVMediaType packet_type, std::vector<AVFrame*>& frame_buffer) {
  while (!packet_queue.empty()) {
    std::unique_ptr<AVPacket, AVPacketDeleter> packet(packet_queue.front());
    packet_queue.pop_front();

    try {
      switch (packet_type) {
        case AVMEDIA_TYPE_AUDIO:
          decode_audio_packet(codec_context, packet.get(), frame_buffer);
          return;
        case AVMEDIA_TYPE_VIDEO:
          decode_video_packet(codec_context, packet.get(), frame_buffer);
          return;
        default: throw std::runtime_error(fmt::format("[{}] Could not decode "
          "packet queue of unimplemented AVMediaType {}.",
          FUNCDINFO, av_get_media_type_string(packet_type)));
      }
    } catch (ffmpeg_error const& e) {
      if (e.get_averror() != AVERROR(EAGAIN)) throw e;
    }
  }
}