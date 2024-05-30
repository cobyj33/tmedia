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
 * Deleter FunctionObjects made to work with C++ STL smart pointers
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