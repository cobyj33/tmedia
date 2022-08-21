#include "ascii_data.h"
#include <cstdint>
#include <cstdlib>
#include <image.h>
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
    int windowWidth, windowHeight;
    get_window_size(&windowWidth, &windowHeight);
    pixel_data* pixelData = get_pixel_data_from_image(fileName);
    if (pixelData == nullptr) {
        std::cout << "Could not get image data from " << fileName;
        return EXIT_FAILURE;
    }

    ascii_image asciiImage = get_image(pixelData->pixels, pixelData->width, pixelData->height, windowWidth, windowHeight);
   pixel_data_free(pixelData); 

    initscr();
    print_ascii_image(&asciiImage);
    refresh();
    char input[1000];
    getch();

    return EXIT_SUCCESS;
}

pixel_data* get_pixel_data_from_image(const char* fileName) {
    AVFormatContext* formatContext(nullptr);
    pixel_data* data = (pixel_data*)malloc(sizeof(pixel_data));

    int result = avformat_open_input(&formatContext, fileName, nullptr, nullptr);
    if (result < 0) {
        std::cout << "Could not open file " << fileName << std::endl;
        avformat_free_context(formatContext);
        pixel_data_free(data);
        return nullptr;
    }

    result = avformat_find_stream_info(formatContext, nullptr);
    if (result < 0) {
        std::cout << "Could not find stream info on " << fileName << std::endl;
        avformat_free_context(formatContext);
        pixel_data_free(data);
        return nullptr;
    }

    const AVCodec* imageDecoder(nullptr);
    
    int imageStreamIndex = -1;
    imageStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &imageDecoder, 0);
    if (imageStreamIndex < 0) {
        std::cout << "Could not find image stream" << std::endl;
        avformat_free_context(formatContext);
        pixel_data_free(data);
        return nullptr;
    }

    AVStream* imageStream = formatContext->streams[imageStreamIndex];
    AVCodecContext* imageCodecContext = avcodec_alloc_context3(imageDecoder);

    result = avcodec_parameters_to_context(imageCodecContext, imageStream->codecpar);
    if (result < 0) {
        std::cout << "Could not set image codec context parameters" << std::endl;
        avformat_free_context(formatContext);
        avcodec_free_context(&imageCodecContext);
        pixel_data_free(data);
        return nullptr;
    }

    result = avcodec_open2(imageCodecContext, imageDecoder, nullptr);
    if (result < 0) {
        std::cout << "Could not open codec context" << std::endl;
        avformat_free_context(formatContext);
        avcodec_free_context(&imageCodecContext);
        pixel_data_free(data);
        return nullptr;
    }

    SwsContext* imageConverter = sws_getContext(
                imageCodecContext->width,
                imageCodecContext->height,
                imageCodecContext->pix_fmt,
                imageCodecContext->width,
                imageCodecContext->height,
                AV_PIX_FMT_GRAY8,
                0,
                nullptr,
                nullptr,
                nullptr
            );
    
    if (imageConverter == NULL) {
        std::cout << "Could not initialize image color converter" << std::endl;
        avformat_free_context(formatContext);
        avcodec_free_context(&imageCodecContext);
        pixel_data_free(data);
        return nullptr;
    }

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
                pixel_data_free(data);
                goto cleanup;
            }
        }

        result = avcodec_receive_frame(imageCodecContext, frame);
        if (result < 0 && result != AVERROR(EAGAIN)) {
            std::cout << "Frame Reading Error..." << result << std::endl;
            pixel_data_free(data);
            goto cleanup;
        } else {
            av_frame_unref(finalFrame);
            finalFrame->width = imageCodecContext->width;
            finalFrame->height = imageCodecContext->height;
            finalFrame->format = AV_PIX_FMT_GRAY8;
            av_frame_get_buffer(finalFrame, 1);
            if (result == AVERROR(EAGAIN)) {
                continue;
            }

            if (result < 0) {
                std::cout << "Frame Reading Error... " << result << std::endl;
                pixel_data_free(data);
                goto cleanup;
            }

            sws_scale(imageConverter, (uint8_t const * const *)frame->data, frame->linesize, 0, imageCodecContext->height, finalFrame->data, finalFrame->linesize);
            av_frame_unref(frame);
            av_packet_unref(packet);
            break;
        }
        av_packet_unref(packet);
    }

    data->width = finalFrame->width;
    data->height = finalFrame->height;
    data->pixels = (uint8_t*)malloc(finalFrame->width * finalFrame->height * sizeof(uint8_t));
    
    for (int i = 0; i < finalFrame->width * finalFrame->height; i++) {
        data->pixels[i] = finalFrame->data[0][i];
    }

    cleanup:
        av_frame_free(&frame);
        av_packet_free(&packet);
        av_frame_free(&finalFrame);
        sws_freeContext(imageConverter);
        avcodec_free_context(&imageCodecContext);
        avformat_close_input(&formatContext);

        return data;
}
