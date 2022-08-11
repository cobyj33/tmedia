#include <cstdint>
#include <cstdlib>
#include <modes.h>
#include <ascii.h>
#include <ncurses.h>
#include <iostream>
#include <ascii_constants.h>



extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include <libavutil/fifo.h>
#include <libavutil/audio_fifo.h>
}

int imageProgram(const char* fileName) {
    AVFormatContext* formatContext(nullptr);

    int result = avformat_open_input(&formatContext, fileName, nullptr, nullptr);
    if (result < 0) {
        std::cout << "Could not open file " << fileName << std::endl;
        return EXIT_FAILURE;
    }

    result = avformat_find_stream_info(formatContext, nullptr);
    if (result < 0) {
        std::cout << "Could not find stream info on " << fileName << std::endl;
        return EXIT_FAILURE;
    }

    const AVCodec* imageDecoder(nullptr);
    
    int imageStreamIndex = -1;
    imageStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &imageDecoder, 0);
    if (imageStreamIndex < 0) {
        std::cout << "Could not find image stream" << std::endl;
        return EXIT_FAILURE;
    }

    AVStream* imageStream = formatContext->streams[imageStreamIndex];
    AVCodecContext* imageCodecContext = avcodec_alloc_context3(imageDecoder);

    result = avcodec_parameters_to_context(imageCodecContext, imageStream->codecpar);
    if (result < 0) {
        std::cout << "Could not set image codec context parameters" << std::endl;
        return EXIT_FAILURE;
    }

    result = avcodec_open2(imageCodecContext, imageDecoder, nullptr);
    if (result < 0) {
        std::cout << "Could not open codec context" << std::endl;
        return EXIT_FAILURE;
    }

    int windowWidth, windowHeight;
    get_window_size(&windowWidth, &windowHeight);
    int IMAGE_WIDTH, IMAGE_HEIGHT;
    get_output_size(imageCodecContext->width, imageCodecContext->height, windowWidth, windowHeight, &IMAGE_WIDTH, &IMAGE_HEIGHT);

    SwsContext* imageConverter = sws_getContext(
                imageCodecContext->width,
                imageCodecContext->height,
                imageCodecContext->pix_fmt,
                IMAGE_WIDTH,
                IMAGE_HEIGHT,
                AV_PIX_FMT_GRAY8,
                0,
                nullptr,
                nullptr,
                nullptr
            );

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* finalFrame = av_frame_alloc();

    while (av_read_frame(formatContext, packet) == 0) {
        result = avcodec_send_packet(imageCodecContext, packet);

        if (result < 0) {
            if (result == AVERROR(EAGAIN)) {
                continue;
            } else {
                std::cout << "Packet Reading Error... " << result << std::endl;
                return EXIT_FAILURE;
            }
        }

        result = avcodec_receive_frame(imageCodecContext, frame);
        if (result < 0 && result != AVERROR(EAGAIN)) {
            std::cout << "Frame Reading Error..." << result << std::endl;
            return EXIT_FAILURE;
        } else {
            av_frame_unref(finalFrame);
            finalFrame->width = IMAGE_WIDTH;
            finalFrame->height = IMAGE_HEIGHT;
            finalFrame->format = AV_PIX_FMT_GRAY8;
            av_frame_get_buffer(finalFrame, 0);
            if (result == AVERROR(EAGAIN)) {
                continue;
            }

            if (result < 0) {
                std::cout << "Frame Reading Error... " << result << std::endl;
                return EXIT_FAILURE;
            }

            sws_scale(imageConverter, (uint8_t const * const *)frame->data, frame->linesize, 0, imageCodecContext->height, finalFrame->data, finalFrame->linesize);
            av_frame_unref(frame);
            av_packet_unref(packet);
            break;
        }
        av_packet_unref(packet);
    }

  av_frame_free(&frame);
  av_packet_free(&packet);
  sws_freeContext(imageConverter);

    initscr();

  ascii_image asciiImage = get_image(finalFrame->data[0], IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT);
  print_ascii_image(&asciiImage);
  refresh();
  char input[1000];
  getch();

  avcodec_free_context(&imageCodecContext);
  avformat_close_input(&formatContext);

  return 0;
}
