
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>


#include "ascii.h"
#include "color.h"
#include "pixeldata.h"
#include "wmath.h"
#include "image.h"
#include "media.h"
#include "decode.h"
#include "boiler.h"
#include "videoconverter.h"
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


PixelData::PixelData(const char* file_name) {
    MediaData media_data(file_name);
    if (!media_data.has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        throw std::runtime_error("[PixelData::PixelData] Could not fetch image of file " + std::string(file_name));
    }
    MediaStream& imageStream = media_data.get_media_stream(AVMEDIA_TYPE_VIDEO);

    AVCodecContext* codec_context = imageStream.get_codec_context();
    VideoConverter image_converter(
            codec_context->width, codec_context->height, AV_PIX_FMT_RGB24,
            codec_context->width, codec_context->height, codec_context->pix_fmt
            );
    AVPacket* packet = av_packet_alloc();
    AVFrame* final_frame = NULL;
    int result;

    std::vector<AVFrame*> original_frame_container;

    while (av_read_frame(media_data.format_context, packet) == 0) {
        if (packet->stream_index != imageStream.get_stream_index())
            continue;

        try {
            original_frame_container = decode_video_packet(codec_context, packet);
            final_frame = image_converter.convert_video_frame(original_frame_container[0]);
            clear_av_frame_list(original_frame_container);
        } catch (ascii::ffmpeg_error e) {
            if (e.get_averror() == AVERROR(EAGAIN)) {
                continue;
            } else {
                av_packet_free((AVPacket**)&packet);
                av_frame_free((AVFrame**)&final_frame);
                throw ascii::ffmpeg_error("ERROR WHILE READING PACKET DATA FROM IMAGE FILE: " + std::string(file_name), result);
            }
        }
    }

    for (int row = 0; row < final_frame->height; row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < final_frame->width; col++) {
            int data_start = row * final_frame->width * 3 + col * 3;
            this->pixels[row].push_back(RGBColor( final_frame->data[0][data_start], final_frame->data[0][data_start + 1], final_frame->data[0][data_start + 2] ));
        }
    }

    av_packet_free((AVPacket**)&packet);
    av_frame_free((AVFrame**)&final_frame);
}

