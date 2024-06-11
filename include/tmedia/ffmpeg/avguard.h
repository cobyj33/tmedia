#ifndef TMEDIA_AVGUARD_H
#define TMEDIA_AVGUARD_H

extern "C" {
  #include <libavutil/version.h>
  #include <libavformat/version.h>
  #include <libavcodec/version.h>
}

/**
 * Various macros to define when various FFmpeg features were added and
 * deprecated. Used to help implement backward compatibility with compiling
 * with older versions of FFmpeg.
*/

/**
 * Macro to determine whether AVFrame has the "duration" field available to
 * use.
 *
 * Source:
 * https://github.com/FFmpeg/FFmpeg/blob/658439934b255215548269116481e2d48da9ee3b/doc/APIchanges#L490
 */
#define HAS_AVFRAME_DURATION LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 30, 100)

/**
 * Macro to determine whether to use the newer AVChannelLayout API of libavutil
 * or the older API.
 *
 * Source:
 * https://github.com/FFmpeg/FFmpeg/blob/658439934b255215548269116481e2d48da9ee3b/doc/APIchanges#L545
 */
#define HAS_AVCHANNEL_LAYOUT LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)


/**
 * Macro to determine whether the AVCodec** parameter on av_find_best_stream
 * should be constified or not.
 *
 * Source:
 * https://github.com/FFmpeg/FFmpeg/blob/658439934b255215548269116481e2d48da9ee3b/doc/APIchanges#L737
*/
#define AV_FIND_BEST_STREAM_CONST_DECODER  LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(59, 0, 100)

/**
 * Determines whether av_register_all should be called before all queries
 * to libavformat are done.
 *
 * Source:
 * https://github.com/FFmpeg/FFmpeg/blob/658439934b255215548269116481e2d48da9ee3b/doc/APIchanges#L1131
*/
#define USE_AV_REGISTER_ALL LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)


/**
 * Determines whether avcodec_register_all should be called before all queries
 * to libavcodec are done.
 *
 * Note that avcodec_register_all was depricated after
 * av_register all was!
 *
 * Source:
 * https://github.com/FFmpeg/FFmpeg/blob/658439934b255215548269116481e2d48da9ee3b/doc/APIchanges#L1136
*/
#define USE_AVCODEC_REGISTER_ALL LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)


/**
 * Determines whether AVInputFormat, AVProbeData, and AVOutputFormat should be
 * declared const in many probing functions.
 *
 * Source:
 * https://github.com/FFmpeg/FFmpeg/blob/658439934b255215548269116481e2d48da9ee3b/doc/APIchanges#L749
*/
#define AVFORMAT_CONST_AVIOFORMAT LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(59, 0, 100)

#endif
