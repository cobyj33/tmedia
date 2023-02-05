
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <ascii.h>
#include "color.h"
#include "pixeldata.h"
#include "wmath.h"
#include <image.h>
#include <media.h>
#include <decode.h>
#include <boiler.h>
#include <videoconverter.h>
#include "except.h"

extern "C" {
#include <ncurses.h>

#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}


PixelData::PixelData(const char* fileName) {
    MediaData media_data(fileName);
    if (!media_data.has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        throw std::runtime_error("[PixelData::PixelData] Could not fetch image of file " + std::string(fileName));
    }
    MediaStream& imageStream = media_data.get_media_stream(AVMEDIA_TYPE_VIDEO);

    AVCodecContext* codecContext = imageStream.info.codecContext;
    VideoConverter imageConverter(
            codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
            codecContext->width, codecContext->height, codecContext->pix_fmt
            );
    AVPacket* packet = av_packet_alloc();
    AVFrame* finalFrame = NULL;
    int result;

    std::vector<AVFrame*> originalFrameContainer;

    while (av_read_frame(media_data.formatContext, packet) == 0) {
        if (packet->stream_index != imageStream.info.stream->index)
            continue;

        try {
            originalFrameContainer = decode_video_packet(codecContext, packet);
            finalFrame = imageConverter.convert_video_frame(originalFrameContainer[0]);
            clear_av_frame_list(originalFrameContainer);
        } catch (ascii::ffmpeg_error e) {
            if (e.get_averror() == AVERROR(EAGAIN)) {
                continue;
            } else {
                av_packet_free((AVPacket**)&packet);
                av_frame_free((AVFrame**)&finalFrame);
                throw ascii::ffmpeg_error("ERROR WHILE READING PACKET DATA FROM IMAGE FILE: " + std::string(fileName), result);
            }
        }
    }

    for (int row = 0; row < finalFrame->height; row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < finalFrame->width; col++) {
            int data_start = row * finalFrame->width * 3 + col * 3;
            this->pixels[row].push_back(RGBColor( finalFrame->data[0][data_start], finalFrame->data[0][data_start + 1], finalFrame->data[0][data_start + 2] ));
        }
    }

    av_packet_free((AVPacket**)&packet);
    av_frame_free((AVFrame**)&finalFrame);
}

