#include "decode.h"

#include "except.h"

#include <vector>
#include <deque>
#include <stdexcept>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

void clear_av_frame_list(std::vector<AVFrame*>& frame_list) {
  for (int i = 0; i < (int)frame_list.size(); i++) {
    av_frame_free(&(frame_list[i]));
  }
  frame_list.clear();
}


std::vector<AVFrame*> decode_video_packet(AVCodecContext* video_codec_context, AVPacket* video_packet) {
  std::vector<AVFrame*> video_frames;
  int result;

  result = avcodec_send_packet(video_codec_context, video_packet);
  if (result < 0) {
    return video_frames;
    // throw ascii::ffmpeg_error("Error while sending video packet: ", result);
  }

  AVFrame* video_frame = av_frame_alloc();

  while (result == 0) {
    result = avcodec_receive_frame(video_codec_context, video_frame);

    if (result < 0) {
      if (result == AVERROR(EAGAIN)) {
        break;
      } else {
        av_frame_free(&video_frame);
        clear_av_frame_list(video_frames);
        return video_frames;
        // throw ascii::ffmpeg_error("Error while receiving video frames during decoding:", result);
      }
    }

    AVFrame* saved_frame = av_frame_alloc();
    result = av_frame_ref(saved_frame, video_frame);
    
    if (result < 0) {
      av_frame_free(&saved_frame);
      av_frame_free(&video_frame);
      clear_av_frame_list(video_frames);
      return video_frames;

      // throw ascii::ffmpeg_error("ERROR while referencing video frames during decoding:", result);
    }

    video_frames.push_back(saved_frame);
    av_frame_unref(video_frame);
  }
  
  av_frame_free(&video_frame);
  return video_frames;
}




std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audio_codec_context, AVPacket* audio_packet) {
  std::vector<AVFrame*> audio_frames;
  int result;

  result = avcodec_send_packet(audio_codec_context, audio_packet);
  if (result < 0) {
    throw ascii::ffmpeg_error("[decode_audio_packet] ERROR WHILE SENDING AUDIO PACKET ", result);
  }

  AVFrame* audio_frame = av_frame_alloc();
  result = avcodec_receive_frame(audio_codec_context, audio_frame);
  if (result < 0) { // if EAGAIN, caller should catch and send more data
    av_frame_free(&audio_frame);
    throw ascii::ffmpeg_error("[decode_audio_packet] FATAL ERROR WHILE RECEIVING AUDIO FRAME ", result);
  }

  while (result == 0) {
    AVFrame* saved_frame = av_frame_alloc();
    result = av_frame_ref(saved_frame, audio_frame); 
    if (result < 0) {
      clear_av_frame_list(audio_frames);
      av_frame_free(&saved_frame);
      av_frame_free(&audio_frame);
      throw ascii::ffmpeg_error("[decode_audio_packet] ERROR WHILE REFERENCING AUDIO FRAMES DURING DECODING ", result);
    }

    av_frame_unref(audio_frame);
    audio_frames.push_back(saved_frame);
    result = avcodec_receive_frame(audio_codec_context, audio_frame);
  }

  av_frame_free(&audio_frame);
  return audio_frames;
}

std::vector<AVFrame*> decode_packet_queue(AVCodecContext* codec_context, std::deque<AVPacket*>& packet_queue, enum AVMediaType packet_type) {
  std::vector<AVFrame*> decoded_frames;
  AVPacket* packet;

  while (!packet_queue.empty()) {
    packet = packet_queue.front();
    packet_queue.pop_front();

    try {
      switch (packet_type) {
        case AVMEDIA_TYPE_AUDIO: decoded_frames = std::move(decode_audio_packet(codec_context, packet)); break;
        case AVMEDIA_TYPE_VIDEO: decoded_frames = std::move(decode_video_packet(codec_context, packet)); break;
        default: throw std::runtime_error("[decode_packet_queue] Could not decode packet queue of unimplemented AVMediaType " + std::string(av_get_media_type_string(packet_type)));
      }
    } catch (ascii::ffmpeg_error const& e) {
      if (e.get_averror() != AVERROR(EAGAIN) || packet_queue.empty()) { // if error is fatal, or the packet list is empty
        av_packet_free(&packet);
        throw e;
      }
    }
    
    av_packet_free(&packet);
    if (decoded_frames.size() > 0) {
      return decoded_frames;
    }
  }

  return decoded_frames;
}