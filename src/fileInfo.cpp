#include <modes.h>
#include <iostream>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/fifo.h>
    #include <libavutil/audio_fifo.h>
}


int fileInfoProgram(const char* fileName) {
    AVFormatContext* formatContext(nullptr);
    int result = avformat_open_input(&formatContext, fileName, nullptr, nullptr);
    if (result < 0) {
        std::cout << "Could not open file " << fileName << std::endl;
        avformat_close_input(&formatContext);
        return EXIT_FAILURE;
    }

    result = avformat_find_stream_info(formatContext, nullptr);
    if (result < 0) {
        std::cout << "Could not get stream info for " << fileName << std::endl;
        return EXIT_FAILURE;
    }

    av_dump_format(formatContext, 0, fileName, 0);

    avformat_close_input(&formatContext);

    return EXIT_SUCCESS;
}