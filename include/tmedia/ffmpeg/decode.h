#ifndef TMEDIA_DECODE_H
#define TMEDIA_DECODE_H

/**
 * @file tmedia/ffmpeg/decode.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Wrapper functions for AVPacket decoding
 * @version 0.5
 * @date 2023-11-19
 *
 * @copyright Copyright (c) 2023
 */

#include <tmedia/ffmpeg/deleter.h>
#include <vector>
#include <memory>

extern "C" {
struct AVFormatContext;
struct AVFrame;
struct AVPacket;
struct AVCodecContext;
struct AVStream;
}

/**
 * The decoding API is made simple with only three exposed
 * functions matching the needs of tmedia. These functions are made with
 * the assumption that an AVFormatContext is only used to decode a single stream
 * of data, fitting the use-case in tmedia for multiple threads to have
 * responsibility for decoding their own streams of a file in parallel with
 * separate AVFormatContext instances pointing to the same file.
 */

/**
 * Decode a single packet given an AVCodecContext and places the returned
 * frames into frame_buffer vector
 *
 * frame_buffer must be cleared to size 0 before it is passed into this
 * function.
 *
 * Note that while EAGAIN may be thrown in an ffmpeg_error from this function,
 * this only means that the next AVPacket must be inputted on the next
 * call to decode_packet.
 *
 * If frame_pool is empty, the functionality of decode_packet is not affected,
 * but decode_packet will have to allocate new AVFrames instead of using old
 * available unreferenced frames.
 *
 * Frames in frame_pool must be unreferenced!
 *
 * This is a much lower-level function, and most use-cases should simply
 * use decode_next_stream_frames in order to fetch the next frame data from
 * a stream.
 *
 * NOTE:
 * while frame_buffer is filled, there should often only be one
 * AVFrame returned if no error is thrown.
 *
 * NOTE:
 * decode_packet may return successfully and just have no frames within
 * frame_buffer.
 *
 * @returns 0 on success, any AVERROR value encountered on error
*/
int decode_packet(AVCodecContext* cctx, AVPacket* pkt, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_buffer, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_pool);

int av_jump_time(AVFormatContext* fctx, AVCodecContext* cctx, AVStream* stream, AVPacket* reading_pkt, double target_time);

/**
 * Decodes the next AVFrame's available in the given stream.
 *
 * decode_next_stream_frames also uses the same pooling interface internally
 * described in the documentation comment given to decode_packet. However,
 * out_frames does not have to be empty, as upon invocation, all frames within
 * out_frames will be unreferenced and moved into frame_pool,
 * which will then be reused as needed by decode_packet.
 *
 * out_frames does not and SHOULD NOT be cleared before passing into this
 * function. This would prevent decode_next_stream_frames from pooling old
 * AVFrame allocations and force decode_packet to allocate new AVFrames to
 * return. Additionally, frame_pool SHOULD NOT be cleared for this same reason.
 */
void decode_next_stream_frames(AVFormatContext* fctx, AVCodecContext* cctx, int stream_idx, AVPacket* reading_pkt, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& out_frames, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_pool);


#endif
