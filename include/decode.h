#ifndef TMEDIA_DECODE_H
#define TMEDIA_DECODE_H

/**
 * @file decode.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Wrapper functions for AVPacket decoding
 * @version 0.5
 * @date 2023-11-19
 * 
 * @copyright Copyright (c) 2023
 */

#include <vector>
#include <deque>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}


/**
 * Clears the AVFrame vector of all it's contents and frees all AVFrame's
 * contained within
 * 
 * @param frame_list 
 * @return void
 */
void clear_avframe_list(std::vector<AVFrame*>& frame_list);

/**
 * Decode a single video packet given an AVCodecContext
 * 
 * throws ffmpeg_error if any error is detected while decoding the
 * given audio packet, including EAGAIN.
 * 
 * Note that while EAGAIN may be thrown in an ffmpeg_error from this function,
 * this only means that the next video AVPacket must be inputted on the next
 * call to decode_video_packet.
 * 
 * Note that while a vector is returned, there should really only be one
 * AVFrame video packet returned if no error is thrown.
 * 
 * @returns a vector of the decoded video frames from the given video packet
 * 
 * The caller is responsible for handling the returned AVFrame* allocated objects
*/
std::vector<AVFrame*> decode_video_packet(AVCodecContext* video_codec_context, AVPacket* video_packet);

/**
 * Decode a single audio packet given an AVCodecContext
 * 
 * throws ffmpeg_error if any error is detected while decoding the
 * given audio packet, including EAGAIN.
 * 
 * Note that while EAGAIN may be thrown in an ffmpeg_error from this function,
 * this only means that the subsequent AVPacket must be inputted on the next
 * call to decode_audio_packet.
 * 
 * @returns a vector of the decoded audio frames from the given audio packet
 * 
 * The caller is responsible for handling the returned AVFrame* allocated objects
*/
std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audio_codec_context, AVPacket* audio_packet);

/**
 * Reads packets from the given packet_queue until there is a successful
 * decoding of frames of type packet_type
 * 
 * The packets in packet_queue must come from a stream of the type denoted
 * by the packet_type parameter.
 * 
 * Note that while decode_packet_queue can fail if there are unrecoverable
 * decoding errors, decode_packet_queue will just return an empty vector if 
 * the packet_queue is empty or the packet_queue runs out of packets without
 * yielding decoded frames
 * 
 * The frames returned from decode_packet_queue are of the type packet_type
 * 
 * The caller is responsible for handling the returned AVFrame* allocated objects
*/
std::vector<AVFrame*> decode_packet_queue(AVCodecContext* codec_context, std::deque<AVPacket*>& packet_queue, enum AVMediaType packet_type);

#endif
