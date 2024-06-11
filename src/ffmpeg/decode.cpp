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

/**
 * While object pooling was definitely unneccesary to implement,
 * I've also never implemented it so this was a cool little thing to do. That
 * is my justification.
*/

void av_frame_move_to_pool(std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_buffer, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_pool) {
  while (!frame_buffer.empty()) {
    av_frame_unref(frame_buffer.back().get());
    frame_pool.push_back(std::move(frame_buffer.back()));
    frame_buffer.pop_back();
  }
}

/**
 * Note that it is possible for av_frame_alloc_from_pool to return a
 * nullptr if av_frame_alloc fails. Make sure to check this from calling
 * code for safety concerns.
 *
 * DESIGN_NOTE:
 * Since av_frame_alloc_from_pool is only
 * called from decode_packet, I designed av_frame_alloc_from_pool to possibly
 * return nullptr rather than throw an exception, so that in decode_packet
 * any AVFrame's in the frame buffer could be moved to the frame
 * pool before returning an error.
*/
std::unique_ptr<AVFrame, AVFrameDeleter> av_frame_alloc_from_pool(std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_pool) {
  if (!frame_pool.empty()) {
    AVFrame* pooled_frame = frame_pool.back().get();
    frame_pool.back().release();
    frame_pool.pop_back();
    return std::unique_ptr<AVFrame, AVFrameDeleter>(pooled_frame);
  } else {
    return std::unique_ptr<AVFrame, AVFrameDeleter>(av_frame_alloc());
  }
}


int decode_packet(AVCodecContext* cctx, AVPacket* pkt, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_buffer, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_pool) {
  int result = avcodec_send_packet(cctx, pkt);
  if (result != 0) {
    return result;
  }

  while (result == 0) {
    std::unique_ptr<AVFrame, AVFrameDeleter> audio_frame = av_frame_alloc_from_pool(frame_pool);
    if (unlikely(audio_frame.get() == nullptr)) {
      av_frame_move_to_pool(frame_buffer, frame_pool);
      return AVERROR(ENOMEM);
    }

    result = avcodec_receive_frame(cctx, audio_frame.get());
    if (result < 0) {
      if (result != AVERROR(EAGAIN))
        av_frame_move_to_pool(frame_buffer, frame_pool);
      av_frame_unref(audio_frame.get());
      frame_pool.push_back(std::move(audio_frame));
      return result;
    }

    frame_buffer.push_back(std::move(audio_frame));
  }

  return result;
}

void decode_next_stream_frames(AVFormatContext* fctx, AVCodecContext* cctx, int stream_idx, AVPacket* reading_pkt, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& out_frames, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_pool) {
  assert(fctx != nullptr);
  assert(cctx != nullptr);
  assert(reading_pkt != nullptr);

  static constexpr int ALLOWED_DECODE_FAILURES = 5;
  int nb_errors_thrown = 0;
  av_frame_move_to_pool(out_frames, frame_pool);

  int res = av_next_stream_packet(fctx, stream_idx, reading_pkt);
  while (res == 0) {
    res = decode_packet(cctx, reading_pkt, out_frames, frame_pool);

    if (res == 0 || (res == AVERROR(EAGAIN) && !out_frames.empty())) {
      return;
    } else if (res != AVERROR(EAGAIN)) {
      nb_errors_thrown++;
      if (nb_errors_thrown > ALLOWED_DECODE_FAILURES) {
        throw std::runtime_error(fmt::format("[{}] Could not decode next {} "
        "packet: \n\t{}", FUNCDINFO, av_get_media_type_string(cctx->codec_type),
        res));
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

  // We just make a local frame pool for decoding up to the desired timestamp.
  // I guess we could pass it in? But I don't see a point since jumping time
  // is such a rare operation
  std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>> frames, frame_pool;

  do {
    decode_next_stream_frames(fctx, cctx, stream->index, reading_pkt, frames, frame_pool);
    for (std::size_t i = 0; i < frames.size(); i++) {
      if (frames[i]->pts * av_q2d(stream->time_base) >= target_time) {
        passed_target_time = true;
        break;
      }
    }
  } while (!passed_target_time && frames.size() > 0);

  return ret;
}
