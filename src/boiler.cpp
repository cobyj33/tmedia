
#include <cstdio>
#include <stdexcept>
#include <string>
#include <iostream>

#include "decode.h"
#include "boiler.h"
#include "except.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/log.h>
}

AVFormatContext* open_format_context(std::string file_name) {
    AVFormatContext* format_context = nullptr;
    int result = avformat_open_input(&format_context, file_name.c_str(), nullptr, nullptr);
    if (result < 0) {
        if (format_context != nullptr) {
            avformat_free_context(format_context);
        }
        throw ascii::ffmpeg_error("Failed to open format context input for " + file_name, result);
    }

    result = avformat_find_stream_info(format_context, NULL);
    if (result < 0) {
        if (format_context != nullptr) {
            avformat_free_context(format_context);
        }
        throw std::runtime_error("Failed to find stream info for " + file_name);
    }

    if (format_context != nullptr) {
        return format_context;
    }
    throw std::runtime_error("Failed to open format context input, unknown error occured");
}

void dump_file_info(const char* file_name) {
    av_log_set_level(AV_LOG_INFO);

    try {
        AVFormatContext* format_context = open_format_context(file_name);
        av_dump_format(format_context, 0, file_name, 0);
        avformat_close_input(&format_context);
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
    }
}
