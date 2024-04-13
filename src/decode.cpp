#include "decode.h"

#include "ffmpeg_error.h"
#include "funcmac.h"
#include "optim.h"

#include <fmt/format.h>
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


std::vector<AVFrame*> decode_video_packet(AVCodecContext* video_codec_context, AVPacket* video_packet) {
  std::vector<AVFrame*> video_frames;
  int result;

  result = avcodec_send_packet(video_codec_context, video_packet);
  if (result < 0) {
    throw ffmpeg_error(fmt::format("[{}] error while sending video packet: ",
                                    FUNCDINFO), result);
  }

  AVFrame* video_frame = av_frame_alloc();
  if (unlikely(video_frame == nullptr)) {
    throw ffmpeg_error("[decode_video_packet] Could not allocate video_frame, "
    "no memory", AVERROR(ENOMEM));
  }

  while (result == 0) {
    result = avcodec_receive_frame(video_codec_context, video_frame);
    if (result < 0) {
      if (result == AVERROR(EAGAIN) && !video_frames.empty()) break;

      av_frame_free(&video_frame);
      clear_avframe_list(video_frames);
      throw ffmpeg_error(fmt::format("[{}] error while receiving video "
      "frames during decoding: ", FUNCDINFO), result);
    }

    AVFrame* saved_frame = av_frame_alloc();
    if (unlikely(saved_frame == nullptr)) {
      av_frame_free(&video_frame);
      clear_avframe_list(video_frames);
      throw ffmpeg_error("[decode_video_packet] Could not allocate saved_frame, no memory", AVERROR(ENOMEM));
    }

    result = av_frame_ref(saved_frame, video_frame);
    if (unlikely(result < 0)) {
      av_frame_free(&saved_frame);
      av_frame_free(&video_frame);
      clear_avframe_list(video_frames);
      throw ffmpeg_error(fmt::format("[{}] error while referencing video "
      "frames during decoding: ", FUNCDINFO), result);
    }

    av_frame_unref(video_frame);
    video_frames.push_back(saved_frame);
  }
  
  av_frame_free(&video_frame);
  return video_frames;
}




std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audio_codec_context, AVPacket* audio_packet) {
  std::vector<AVFrame*> audio_frames;
  int result;

  result = avcodec_send_packet(audio_codec_context, audio_packet);
  if (result < 0) {
    throw ffmpeg_error(fmt::format("[{}] error while sending audio packet ",
                                    FUNCDINFO), result);
  }

  AVFrame* audio_frame = av_frame_alloc();
  if (unlikely(audio_frame == nullptr)) {
    throw ffmpeg_error("[decode_audio_packet] Could not allocate audio_frame, "
    "no memory", AVERROR(ENOMEM));
  }

  while (result == 0) {
    result = avcodec_receive_frame(audio_codec_context, audio_frame);
    if (result < 0) {
      if (result == AVERROR(EAGAIN) && !audio_frames.empty()) break;

      av_frame_free(&audio_frame);
      clear_avframe_list(audio_frames);
      throw ffmpeg_error(fmt::format("[{}] error while receiving audio "
      "frames during decoding: ", FUNCDINFO), result);
    }

    AVFrame* saved_frame = av_frame_alloc();
    if (unlikely(saved_frame == nullptr)) {
      av_frame_free(&audio_frame);
      clear_avframe_list(audio_frames);
      throw ffmpeg_error("[decode_audio_packet] Could not allocate "
      "saved_frame, no memory", AVERROR(ENOMEM));
    } 

    result = av_frame_ref(saved_frame, audio_frame); 
    if (unlikely(result < 0)) {
      clear_avframe_list(audio_frames);
      av_frame_free(&saved_frame);
      av_frame_free(&audio_frame);
      throw ffmpeg_error(fmt::format("[{}] error while referencing audio "
                                "frames during decoding ", FUNCDINFO), result);
    }

    av_frame_unref(audio_frame);
    audio_frames.push_back(saved_frame);
  }

  av_frame_free(&audio_frame);
  return audio_frames;
}

std::vector<AVFrame*> decode_packet_queue(AVCodecContext* codec_context, std::deque<AVPacket*>& packet_queue, enum AVMediaType packet_type) {
  while (!packet_queue.empty()) {
    AVPacket* packet = packet_queue.front();
    packet_queue.pop_front();

    try {
      switch (packet_type) {
        case AVMEDIA_TYPE_AUDIO: {
          std::vector<AVFrame*> dec_frames = decode_audio_packet(codec_context, packet);
          av_packet_free(&packet);
          return dec_frames;
        } break;
        case AVMEDIA_TYPE_VIDEO: {
          std::vector<AVFrame*> dec_frames = decode_video_packet(codec_context, packet);
          av_packet_free(&packet);
          return dec_frames;
        } break;
        default: throw std::runtime_error(fmt::format("[{}] Could not decode "
          "packet queue of unimplemented AVMediaType {}.",
          FUNCDINFO, av_get_media_type_string(packet_type)));
      }
    } catch (ffmpeg_error const& e) {
      if (e.get_averror() != AVERROR(EAGAIN)) { // if error is fatal, or the packet list is empty
        av_packet_free(&packet);
        throw e;
      }
    }
    
    av_packet_free(&packet);
  }

  return {};
}