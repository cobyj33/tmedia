#pragma once
#ifndef TMEDIA_FFMPEG_DELETER_H
#define TMEDIA_FFMPEG_DELETER_H

extern "C" {
struct AVFormatContext;
struct AVFrame;
struct AVPacket;
struct AVCodecContext;
}

/**
 * Deleter FunctionObjects made to work with C++ STL smart pointers.
 *
 * These are largely necessary for guaranteeing that allocated FFmpeg structs
 * will never leak data even in scopes in which exceptions may be thrown. This
 * is especially true for many STL functions which can reallocate and therefore
 * call std::bad_alloc, causing a memory leak if that were to happen.
 *
 * Note that all deleter objects are safe to be used with smart pointers that
 * may contain nullptr
*/

/**
 * The allocx functions here are used as a wrapper to throw std::bad_alloc
 * if any allocation error were to happen rather than checking for nullptr
 * after every call to av_packet_alloc and av_frame_alloc.
 */

/**
 * Wrapper for av_packet_alloc, Throws std::bad_alloc whenever
 * av_packet_alloc returns nullptr. Therefore, the returned AVPacket
 * pointer is guaranteed to not be nullptr.
 */
AVPacket* av_packet_allocx();

/**
 * Wrapper for av_frame_alloc. Throws std::bad_alloc whenever
 * av_frame_alloc returns nullptr. Therefore, the returned AVFrame
 * pointer is guaranteed to not be nullptr.
 */
AVFrame* av_frame_allocx();

/**
 * A Deleter Function Object which is used in smart pointers to call
 * avformat_close_input on a given AVFormatContext pointer
 * (not avformat_free_context)
 *
 * This is safe to use with any AVFormatContext opened with fctx_open from
 * tmedia/ffmpeg/boiler.h
 */
class AVFormatContextDeleter {
  public:
    constexpr AVFormatContextDeleter() noexcept = default;
    void operator()(AVFormatContext* __ptr) const;
};

class AVCodecContextDeleter {
  public:
    constexpr AVCodecContextDeleter() noexcept = default;
    void operator()(AVCodecContext* __ptr) const;
};

class AVPacketDeleter {
  public:
    constexpr AVPacketDeleter() noexcept = default;
    void operator()(AVPacket* __ptr) const;
};

class AVFrameDeleter {
  public:
    constexpr AVFrameDeleter() noexcept = default;
    void operator()(AVFrame* __ptr) const;
};

#endif
