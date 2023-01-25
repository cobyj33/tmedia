
#include "decode.h"
#include <boiler.h>
#include <cstdio>

extern "C" {
#include <libavformat/avformat.h>
}


AVFormatContext* open_format_context(const char* fileName, int* result) {
    AVFormatContext* formatContext = NULL;
    *result = avformat_open_input(&formatContext, fileName, NULL, NULL);
    if (*result < 0) {
        if (formatContext == NULL) {
            avformat_free_context(formatContext);
        }
        return NULL;
    }

    *result = avformat_find_stream_info(formatContext, NULL);
    if (*result < 0) {
        avformat_free_context(formatContext);
        return NULL;
    }

    return formatContext;
}




