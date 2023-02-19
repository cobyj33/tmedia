#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <vector>

#include "info.h"
#include "boiler.h"

extern "C" {
#include <libavutil/log.h>
#include <libavformat/avformat.h>
}

int fileInfoProgram(const char* file_name) {
    av_log_set_level(AV_LOG_INFO);

    try {
        AVFormatContext* format_context = open_format_context(file_name);
        av_dump_format(format_context, 0, file_name, 0);
        avformat_close_input(&format_context);
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
