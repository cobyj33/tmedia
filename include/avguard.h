#ifndef TMEDIA_AVGUARD_H
#define TMEDIA_AVGUARD_H

extern "C" {
  #include <libavutil/version.h>
  #include <libavformat/version.h>
  #include <libavcodec/version.h>
}

/**
 * Various macros to define when various FFmpeg features were added and 
 * deprecated
*/

#define HAS_AVFRAME_DURATION LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 30, 100)
#define HAS_AVCHANNEL_LAYOUT LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
#define AV_FIND_BEST_STREAM_CONST_DECODER  LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(59, 0, 100)
#define USE_AV_REGISTER_ALL LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
#define USE_AVCODEC_REGISTER_ALL LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)
#define AVFORMAT_CONST_AVIOFORMAT LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(59, 0, 100)

#endif