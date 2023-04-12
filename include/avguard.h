#ifndef ASCII_VIDEO_AVGUARD
#define ASCII_VIDEO_AVGUARD

extern "C" {
    #include <libavutil/version.h>
}

/** AVFrame.duration introduced in libavutil version 57.30.100 */
#define HAS_AVFRAME_DURATION (LIBAVUTIL_VERSION_MAJOR > 57 || (LIBAVUTIL_VERSION_MAJOR == 57 && LIBAVUTIL_VERSION_MINOR > 30) || LIBAVUTIL_VERSION_MAJOR == 57 && LIBAVUTIL_VERSION_MINOR == 30 && LIBAVUTIL_VERSION_PATCH >= 100)


#endif