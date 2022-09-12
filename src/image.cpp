#include <media.h>
#include <renderer.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <image.h>
#include <decode.h>
#include <ascii.h>
#include <ncurses.h>
#include <iostream>
#include <boiler.h>

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
    erase();
    pixel_data* pixelData = get_pixel_data_from_image(fileName);
    if (pixelData == nullptr) {
        std::cout << "Could not get image data from " << fileName;
        endwin();
        return EXIT_FAILURE;
    }

    ascii_image textImage = get_ascii_image_bounded(pixelData, COLS, LINES);
    print_ascii_image_full(&textImage);
    pixel_data_free(pixelData); 
    refresh();
    getch();

    endwin();
    return EXIT_SUCCESS;
}

pixel_data* get_pixel_data_from_image(const char* fileName) {
    MediaData* mediaData = media_data_alloc(fileName);
    if (mediaData == nullptr) {
        std::cout << "Could not fetch media data of " << fileName << std::endl;
        return nullptr;
    }

    MediaStream* imageStream = get_media_stream(mediaData, AVMEDIA_TYPE_VIDEO);
    if (imageStream == nullptr) {
        std::cerr << "Could not fetch image stream of " << fileName << std::endl;
        return nullptr;
    }

    AVCodecContext* codecContext = imageStream->info->codecContext;
    VideoConverter* imageConverter = get_video_converter(
            codecContext->width, codecContext->height, AV_PIX_FMT_GRAY8,
            codecContext->width, codecContext->height, codecContext->pix_fmt
            );
    
    if (imageConverter == nullptr) {
        std::cout << "Could not initialize image color converter" << std::endl;
        media_data_free(mediaData);
        return nullptr;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* finalFrame = nullptr;

    auto cleanup = [mediaData, packet, finalFrame, imageConverter](){
        av_packet_free((AVPacket**)&packet);
        av_frame_free((AVFrame**)&finalFrame);
        free_video_converter(imageConverter);
        media_data_free(mediaData);
    };

    int result;
    while (av_read_frame(mediaData->formatContext, packet) == 0) {
        int nb_out;
        if (packet->stream_index != imageStream->info->stream->index)
            continue;

        AVFrame** originalFrame = decode_video_packet(codecContext, packet, &result, &nb_out);
        if (result == AVERROR(EAGAIN)) {
            continue;
        } else if (result < 0) {
            std::cout << "ERROR WHILE READING PACKET DATA FROM IMAGE " << fileName << std::endl;
            cleanup();
            free_frame_list(originalFrame, nb_out);
            return nullptr;
        } else {
            finalFrame = convert_video_frame(imageConverter, originalFrame[0]);
            free_frame_list(originalFrame, nb_out);
            break;
        }
    }

    if (finalFrame == nullptr) {
        cleanup();
        return nullptr;
    }

    pixel_data* data = pixel_data_alloc(finalFrame->width, finalFrame->height);
    for (int i = 0; i < finalFrame->width * finalFrame->height; i++) {
        data->pixels[i] = finalFrame->data[0][i];
    }

    cleanup();
    return data;
}

pixel_data* pixel_data_alloc_from_frame(AVFrame* videoFrame) {
    pixel_data* data = pixel_data_alloc(videoFrame->width, videoFrame->height);
    for (int i = 0; i < videoFrame->width * videoFrame->height; i++) {
        data->pixels[i] = videoFrame->data[0][i];
    }
    return data;
}

pixel_data* copy_pixel_data(pixel_data* data) {
    pixel_data* new_data = pixel_data_alloc(data->width, data->height);
    for (int i = 0; i < data->width * data->height; i++) {
        new_data->pixels[i] = data->pixels[i];
    }
    return new_data;
}

pixel_data* pixel_data_alloc(int width, int height) {
  pixel_data* pixelData = (pixel_data*)malloc(sizeof(pixel_data));
  pixelData->pixels = (uint8_t*)malloc(width * height * sizeof(uint8_t));
  pixelData->width = width;
  pixelData->height = height;

  for (int i = 0; i < width * height; i++) {
      pixelData->pixels[i] = (uint8_t)0;
  }

  return pixelData;
}

void pixel_data_free(pixel_data* pixelData) {
  free(pixelData->pixels);
  free(pixelData);
}
