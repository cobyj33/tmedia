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
 * Decode a single video packet given an AVCodecContext and places the values
 * into frame_buffer vector
 * 
 * frame_buffer must be cleared to size 0 before it is passed into this
 * function
 * 
 * throws ffmpeg_error if any error is detected while decoding the
 * given audio packet, including EAGAIN.
 * 
 * Note that while EAGAIN may be thrown in an ffmpeg_error from this function,
 * this only means that the next video AVPacket must be inputted on the next
 * call to decode_video_packet.
 * 
 * Note that while frame_buffer is filled, there should often only be one
 * AVFrame video packet returned if no error is thrown.
*/
void decode_video_packet(AVCodecContext* video_codec_context, AVPacket* video_packet, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_buffer);

/**
 * Decode a single audio packet given an AVCodecContext and places the values
 * into frame_buffer vector
 * 
 * frame_buffer must be cleared to size 0 before it is passed into this
 * function
 * 
 * throws ffmpeg_error if any error is detected while decoding the
 * given audio packet, including EAGAIN.
 * 
 * Note that while EAGAIN may be thrown in an ffmpeg_error from this function,
 * this only means that the next audio AVPacket must be inputted on the next
 * call to decode_audio_packet.
 * 
 * Note that while frame_buffer is filled, there should often only be one
 * AVFrame audio packet returned if no error is thrown.
*/
void decode_audio_packet(AVCodecContext* audio_codec_context, AVPacket* audio_packet, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& frame_buffer);

int av_jump_time(AVFormatContext* fctx, AVCodecContext* cctx, AVStream* stream, AVPacket* reading_pkt, double target_time);
void decode_next_stream_frames(AVFormatContext* fctx, AVCodecContext* cctx, int stream_idx, AVPacket* reading_pkt, std::vector<std::unique_ptr<AVFrame, AVFrameDeleter>>& out_frames);


#endif
