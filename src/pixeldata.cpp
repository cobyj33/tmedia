#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <memory>

#include "pixeldata.h"
#include "ascii.h"
#include "color.h"
#include "pixeldata.h"
#include "wmath.h"
#include "scale.h"
#include "media.h"
#include "decode.h"
#include "boiler.h"
#include "videoconverter.h"
#include "except.h"
#include "streamdata.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

// Accepts empty matrices
template <typename T>
bool is_rectangular_vector_matrix(std::vector<std::vector<T>> vector_2d) {
    if (vector_2d.size() == 0) {
        return true;
    }
    int width = vector_2d[0].size();

    for (int row = 0; row < vector_2d.size(); row++) {
        if (vector_2d[row].size() != width) {
            return false;
        }
    }

    return true;
}


PixelData::PixelData(std::vector<std::vector<RGBColor> >& raw_rgb_data) {
    if (!is_rectangular_vector_matrix(raw_rgb_data)) {
        throw std::runtime_error("Cannot initialize pixel data with non-rectangular matrix");
    }

    for (int row = 0; row < raw_rgb_data.size(); row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < raw_rgb_data[row].size(); col++) {
            this->pixels[row].push_back(raw_rgb_data[row][col]);
        }
    }
}

bool PixelData::in_bounds(int row, int col) const {
    return row >= 0 && col >= 0 && row < this->get_height() && col < this->get_width();
}

PixelData::PixelData(std::vector<std::vector<uint8_t> >& raw_grayscale_data) {
    if (!is_rectangular_vector_matrix(raw_grayscale_data)) {
        throw std::runtime_error("Cannot initialize pixel data with non-rectangular matrix");
    }

    for (int row = 0; row < raw_grayscale_data.size(); row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < raw_grayscale_data[row].size(); col++) {
            this->pixels[row].push_back(RGBColor(raw_grayscale_data[row][col]));
        }
    }
}

PixelData::PixelData(AVFrame* video_frame) {
        
    for (int row = 0; row < video_frame->height; row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < video_frame->width; col++) {
            if ((AVPixelFormat)video_frame->format == AV_PIX_FMT_GRAY8) {
                this->pixels[row].push_back(RGBColor(video_frame->data[0][row * video_frame->width + col]));
            } else if ((AVPixelFormat)video_frame->format == AV_PIX_FMT_RGB24) {
                int data_start = row * video_frame->width * 3 + col * 3;
                this->pixels[row].push_back(RGBColor( video_frame->data[0][data_start], video_frame->data[0][data_start + 1], video_frame->data[0][data_start + 2] ));
            } else {
                throw std::runtime_error("Passed in AVFrame with unimplemeted format, only supported formats for initializing from AVFrame are AV_PIX_FMT_GRAY8 and AV_PIX_FMT_RGB24");
            }

        }
    }

}


RGBColor PixelData::get_avg_color_from_area(int row, int col, int width, int height) const {
    if (width * height <= 0) {
        throw std::runtime_error("Cannot get average color from an area with negative dimensions: " + std::to_string(width) + " x " + std::to_string(height));
    }
    if (width * height == 0) {
        throw std::runtime_error("Cannot get average color from an area with dimension of 0: " + std::to_string(width) + " x " + std::to_string(height));
    }

    std::vector<RGBColor> colors;
    for (int curr_row = row; curr_row < row + height; curr_row++) {
        for (int curr_col = col; curr_col < col + width; curr_col++) {
            if (this->in_bounds(curr_row, curr_col)) {
                colors.push_back(this->at(curr_row, curr_col));
            }
        }
    }

    if (colors.size() > 0) {
        return get_average_color(colors);
    }
    return RGBColor::WHITE;
    // throw std::runtime_error("Cannot get average color of out of bounds area");
}

RGBColor PixelData::get_avg_color_from_area(double row, double col, double width, double height) const {
    return this->get_avg_color_from_area((int)std::floor(row), (int)std::floor(col), (int)std::ceil(width), (int)std::ceil(height));
}

PixelData::PixelData(const PixelData& other) {
    for (int row = 0; row < other.get_height(); row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < other.get_width(); col++) {
            this->pixels[row].push_back(other.at(row, col));
        }
    }
}


int PixelData::get_width() const {
    if (this->pixels.size() == 0) {
        return 0;
    }
    return this->pixels[0].size();
}

int PixelData::get_height() const {
    return this->pixels.size();
}

PixelData PixelData::scale(double amount) const {
    if (amount == 0) {
        return PixelData();
    } else if (amount < 0) {
        throw std::runtime_error("Scaling Pixel data by negative amount is currently not supported");
    }

    int new_width = this->get_width() * amount;
    int new_height = this->get_height() * amount;
    std::vector<std::vector<RGBColor>> new_pixels;

    double box_width = 1 / amount;
    double box_height = 1 / amount;

    for (double new_row = 0; new_row < new_height; new_row++) {
        new_pixels.push_back(std::vector<RGBColor>());
        for (double new_col = 0; new_col < new_width; new_col++) {
            RGBColor color = this->get_avg_color_from_area( new_row * box_height, new_col * box_width, box_width, box_height );
            new_pixels[new_row].push_back(color);
        }
    }
    return PixelData(new_pixels);
}

PixelData PixelData::bound(int width, int height) const {
    if (this->get_width() <= width && this->get_height() <= height) {
        return PixelData(*this);
    }
    double scale_factor = get_scale_factor(this->get_width(), this->get_height(), width, height);
    return this->scale(scale_factor);
}

bool PixelData::equals(const PixelData& other) const {
    if (this->get_width() != other.get_width() || this->get_height() != other.get_height()) {
        return false;
    }

    for (int row = 0; row < this->get_height(); row++) {
        for (int col = 0; col < this->get_width(); col++) {
            RGBColor other_color = other.at(row, col);
            if (!this->at(row, col).equals(other_color)) {
                return false;
            }
        }
    }
    return true;
}

RGBColor PixelData::at(int row, int col) const {
    return this->pixels[row][col];
}

PixelData::PixelData(const char* file_name) {
    AVFormatContext* format_context;
    format_context = open_format_context(std::string(file_name));

    std::vector<enum AVMediaType> media_types = { AVMEDIA_TYPE_VIDEO };
    std::unique_ptr<std::vector<std::unique_ptr<StreamData>>> media_streams = std::move(get_stream_datas(format_context, media_types));

    if (!has_av_media_stream(media_streams, AVMEDIA_TYPE_VIDEO)) {
        throw std::runtime_error("[PixelData::PixelData(const char* file_name)] Could not fetch image of file " + std::string(file_name));
    }
    
    StreamData& imageStream = get_av_media_stream(media_streams, AVMEDIA_TYPE_VIDEO);

    AVCodecContext* codec_context = imageStream.get_codec_context();
    VideoConverter image_converter(
            codec_context->width, codec_context->height, AV_PIX_FMT_RGB24,
            codec_context->width, codec_context->height, codec_context->pix_fmt
            );
    AVPacket* packet = av_packet_alloc();
    AVFrame* final_frame = NULL;
    int result;

    std::vector<AVFrame*> original_frame_container;

    while (av_read_frame(format_context, packet) == 0) {
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
    avformat_free_context(format_context);
}