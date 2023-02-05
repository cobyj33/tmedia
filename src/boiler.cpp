
#include "decode.h"
#include <boiler.h>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <except.h>

extern "C" {
#include <libavformat/avformat.h>
}


AVFormatContext* open_format_context(std::string fileName) {
    AVFormatContext* formatContext = nullptr;
    int result = avformat_open_input(&formatContext, fileName.c_str(), nullptr, nullptr);
    if (result < 0) {
        if (formatContext != nullptr) {
            avformat_free_context(formatContext);
        }
        throw ascii::ffmpeg_error("Failed to open format context input for " + fileName, result);
    }

    result = avformat_find_stream_info(formatContext, NULL);
    if (result < 0) {
        if (formatContext != nullptr) {
            avformat_free_context(formatContext);
        }
        throw std::runtime_error("Failed to find stream info for " + fileName);
    }

    if (formatContext != nullptr) {
        return formatContext;
    }
    throw std::runtime_error("Failed to open format context input, unknown error occured");
}




