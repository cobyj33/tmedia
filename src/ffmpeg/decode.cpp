#include <tmedia/ffmpeg/decode.h>

#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/ffmpeg/deleter.h>
#include <tmedia/util/defines.h>

#include <fmt/format.h>

#include <memory>
#include <vector>
#include <deque>
#include <stdexcept>
#include <string>
#include <cassert>


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

void decode_next_stream_frames(AVFormatContext* fctx, AVCodecContext* cctx, int stream_idx, AVPacket* reading_pkt, std::vector<AVFrame*>& out_frames) {
  static constexpr int ALLOWED_DECODE_FAILURES = 5;
  int nb_errors_thrown = 0;

  int res = av_next_stream_packet(fctx, stream_idx, reading_pkt);
  while (res == 0) {
    try {
      if (cctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        decode_audio_packet(cctx, reading_pkt, out_frames);
        return;
      } else if (cctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        decode_video_packet(cctx, reading_pkt, out_frames);
        return;
      } else {
        throw std::runtime_error(fmt::format("[{}] Could not decode "
        "packet queue of unimplemented AVMediaType {}.",
        FUNCDINFO, av_get_media_type_string(cctx->codec_type)));
      }
    } catch (ffmpeg_error const& e) {
      if (e.averror != AVERROR(EAGAIN)) {
        nb_errors_thrown++;
        if (nb_errors_thrown > ALLOWED_DECODE_FAILURES) {
          throw std::runtime_error(fmt::format("[{}] Could not decode next {} "
          "packet: \n\t{}", FUNCDINFO, av_get_media_type_string(cctx->codec_type),
          e.what()));
        }
      }
    }

    res = av_next_stream_packet(fctx, stream_idx, reading_pkt);
  }
}

int av_jump_time(AVFormatContext* fctx, AVCodecContext* cctx, AVStream* stream, AVPacket* reading_pkt, double target_time) {
  assert(target_time >= 0.0 && target_time <= (fctx->duration / AV_TIME_BASE));

  int ret = avformat_seek_file(fctx, -1, 0.0,
    target_time * AV_TIME_BASE, target_time * AV_TIME_BASE, 0);

  if (ret < 0)
    return ret;

  avcodec_flush_buffers(cctx);
  bool passed_target_time = false;
  std::vector<AVFrame*> frames;

  do {
    clear_avframe_list(frames);
    decode_next_stream_frames(fctx, cctx, stream->index, reading_pkt, frames);
    for (std::size_t i = 0; i < frames.size(); i++) {
      if (frames[i]->pts * av_q2d(stream->time_base) >= target_time) {
        passed_target_time = true;
        break;
      }
    }
  } while (!passed_target_time && frames.size() > 0);

  clear_avframe_list(frames);
  return ret;
}
