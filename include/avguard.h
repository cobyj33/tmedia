#ifndef ASCII_VIDEO_AVGUARD
#define ASCII_VIDEO_AVGUARD

extern "C" {
    #include <libavutil/version.h>
    #include <libavformat/version.h>
    #include <libavcodec/version.h>
}

/** AVFrame.duration introduced in libavutil version 57.30.100 */

#define HAS_AVFRAME_DURATION LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 30, 100)
#define HAS_AVCHANNEL_LAYOUT LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
#define AV_FIND_BEST_STREAM_CONST_DECODER  LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(59, 0, 100)
#define USE_AV_REGISTER_ALL LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
#define USE_AVCODEC_REGISTER_ALL LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)

#endif