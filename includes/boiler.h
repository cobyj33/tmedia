#ifndef ASCII_VIDEO_BOILER
#define ASCII_VIDEO_BOILER

extern "C" {
#include <libavformat/avformat.h>
}

AVFormatContext* open_format_context(const char* fileName, int* result);

#endif
