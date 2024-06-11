#ifndef TMEDIA_BOILER_H
#define TMEDIA_BOILER_H

/**
 * @file tmedia/ffmpeg/boiler.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Boilerplate reducing functions for various miscellaneous FFmpeg operations
 * @version 0.1
 * @date 2023-11-19
 *
 * @copyright Copyright (c) 2023
 */

#include <tmedia/media/mediatype.h>
#include <tmedia/ffmpeg/deleter.h>
#include <tmedia/ffmpeg/avguard.h> // for HAS_AVCHANNEL_LAYOUT

#include <optional>
#include <memory> // std::unique_ptr

#include <filesystem>


extern "C" {
#include <libavutil/avutil.h> // for enum AVMediaType
struct AVFormatContext;
struct AVInputFormat;
struct AVCodecContext;
struct AVStream;
#if HAS_AVCHANNEL_LAYOUT
struct AVChannelLayout;
#endif
}

/**
 * @brief Opens and returns an AVFormat Context
 *
 * @note The returned AVFormatContext is guaranteed to be non-null
 * @note The returned AVFormatContext must be freed with avformat_close_input, not just avformat_free_context
 * @throws std::runtime_error if the format context could not be opened for the corresponding file name
 * @throws std::runtime_error if the stream information for the format context could not be found
 *
 * @param path The path of the file to open from the current working directory
 * @return An AVFormatContext pointer representing the opened media file
 */
std::unique_ptr<AVFormatContext, AVFormatContextDeleter> open_fctx(const std::filesystem::path& path);

struct OpenCCTXRes {
  std::unique_ptr<AVCodecContext, AVCodecContextDeleter> cctx;
  AVStream* avstr;
};

/**
 * Opens an AVCodecContext from an AVFormatContext.
 *
 * @returns A owning unique_ptr to the AVCodecContext and a pointer to the
 * AVStream which the AVCodecContext has been configured to decode frames from.
 *
 * The caller must not free the returned AVStream directly, as it is owned by
 * the AVFormatContext instance.
 */
OpenCCTXRes open_cctx(AVFormatContext* fmt_ctx, enum AVMediaType media_type);


double avstr_get_start_time(AVStream* avstr);

/**
 * This function calls av_packet_unref on packet before doing anything
*/
int av_next_stream_packet(AVFormatContext* fctx, int stream_idx, AVPacket* pkt);

#if HAS_AVCHANNEL_LAYOUT
AVChannelLayout* cctx_get_ch_layout(AVCodecContext* cctx);
#else
int64_t cctx_get_ch_layout(AVCodecContext* cctx);
#endif

int cctx_get_nb_channels(AVCodecContext* cctx);

bool avformat_context_has_media_stream(AVFormatContext* fmt_ctx, enum AVMediaType media_type);

MediaType media_type_from_avformat_context(AVFormatContext* fmt_ctx);
std::optional<MediaType> media_type_from_iformat(const AVInputFormat* iformat);

#endif
