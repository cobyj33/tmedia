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
bool is_rectangular_vector_matrix(std::vector<std::vector<T>>& vector_2d) {
  if (vector_2d.size() == 0) {
    return true;
  }
  std::size_t width = vector_2d[0].size();

  for (int row = 0; row < (int)vector_2d.size(); row++) {
    if (vector_2d[row].size() != width) {
      return false;
    }
  }

  return true;
}

PixelData::PixelData(std::vector<std::vector<RGBColor>>& raw_rgb_data) {
  if (!is_rectangular_vector_matrix(raw_rgb_data)) {
    throw std::runtime_error("Cannot initialize pixel data with non-rectangular matrix");
  }

  this->m_height = raw_rgb_data.size();
  this->m_width = 0;

  if (raw_rgb_data.size() > 0) {
    this->m_width = raw_rgb_data[0].size();
    this->pixels.reserve(raw_rgb_data.size() * raw_rgb_data[0].size());
    for (int row = 0; row < (int)raw_rgb_data.size(); row++) {
      for (int col = 0; col < (int)raw_rgb_data[0].size(); col++) {
        this->pixels.push_back(raw_rgb_data[row][col]);
      }
    }
  }
}

PixelData::PixelData(std::vector<std::vector<uint8_t> >& raw_grayscale_data) {
  if (!is_rectangular_vector_matrix(raw_grayscale_data)) {
    throw std::runtime_error("Cannot initialize pixel data with non-rectangular matrix");
  }

  this->m_height = raw_grayscale_data.size();
  this->m_width = 0;

  if (raw_grayscale_data.size() > 0) {
    this->m_width = raw_grayscale_data[0].size();
    this->pixels.reserve(raw_grayscale_data.size() * raw_grayscale_data[0].size());
    for (int row = 0; row < (int)raw_grayscale_data.size(); row++) {
      for (int col = 0; col < (int)raw_grayscale_data[0].size(); col++) {
        this->pixels.push_back(RGBColor(raw_grayscale_data[row][col]));
      }
    }
  }
}

PixelData::PixelData(AVFrame* video_frame) {
  this->pixels.reserve(video_frame->width * video_frame->height);
  this->m_width = video_frame->width;
  this->m_height = video_frame->height;

  for (int row = 0; row < video_frame->height; row++) {
    for (int col = 0; col < video_frame->width; col++) {
      if ((AVPixelFormat)video_frame->format == AV_PIX_FMT_GRAY8) {
        this->pixels.push_back(RGBColor(video_frame->data[0][row * video_frame->width + col]));
      } else if ((AVPixelFormat)video_frame->format == AV_PIX_FMT_RGB24) {
        int data_start = row * video_frame->width * 3 + col * 3;
        this->pixels.push_back(RGBColor( video_frame->data[0][data_start], video_frame->data[0][data_start + 1], video_frame->data[0][data_start + 2] ));
      } else {
        throw std::runtime_error("Passed in AVFrame with unimplemeted format, only supported formats for initializing from AVFrame are AV_PIX_FMT_GRAY8 and AV_PIX_FMT_RGB24");
      }
    }
  }
}

void PixelData::operator=(AVFrame* video_frame) {
  this->pixels.clear();
  this->pixels.reserve(video_frame->width * video_frame->height);
  this->m_width = video_frame->width;
  this->m_height = video_frame->height;

  for (int row = 0; row < video_frame->height; row++) {
    for (int col = 0; col < video_frame->width; col++) {
      if ((AVPixelFormat)video_frame->format == AV_PIX_FMT_GRAY8) {
        this->pixels.push_back(RGBColor(video_frame->data[0][row * video_frame->width + col]));
      } else if ((AVPixelFormat)video_frame->format == AV_PIX_FMT_RGB24) {
        int data_start = row * video_frame->width * 3 + col * 3;
        this->pixels.push_back(RGBColor( video_frame->data[0][data_start], video_frame->data[0][data_start + 1], video_frame->data[0][data_start + 2] ));
      } else {
        throw std::runtime_error("Passed in AVFrame with unimplemeted format, only supported formats for initializing from AVFrame are AV_PIX_FMT_GRAY8 and AV_PIX_FMT_RGB24");
      }
    }
  }
}

PixelData::PixelData(const PixelData& pix_data) {
  this->pixels.reserve(pix_data.get_width() * pix_data.get_height());
  this->m_width = pix_data.get_width();
  this->m_height = pix_data.get_height();
  for (int row = 0; row < pix_data.get_height(); row++) {
    for (int col = 0; col < pix_data.get_width(); col++) {
      this->pixels.push_back(pix_data.at(row, col));
    }
  }
}

void PixelData::operator=(const PixelData& pix_data) {
  this->pixels.clear();
  this->pixels.reserve(pix_data.get_width() * pix_data.get_height());
  this->m_width = pix_data.get_width();
  this->m_height = pix_data.get_height();
  for (int row = 0; row < pix_data.get_height(); row++) {
    for (int col = 0; col < pix_data.get_width(); col++) {
      this->pixels.push_back(pix_data.at(row, col));
    }
  }
}

bool PixelData::in_bounds(int row, int col) const {
  return row >= 0 && col >= 0 && row < this->m_height && col < this->m_width;
}

RGBColor PixelData::get_avg_color_from_area(double row, double col, double width, double height) const {
  return this->get_avg_color_from_area((int)std::floor(row), (int)std::floor(col), (int)std::ceil(width), (int)std::ceil(height));
}

int PixelData::get_width() const {
  return this->m_width;
}

int PixelData::get_height() const {
  return this->m_height;
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

bool PixelData::equals(const PixelData& pix_data) const {
  if (this->get_width() != pix_data.get_width() || this->get_height() != pix_data.get_height()) {
    return false;
  }

  for (int row = 0; row < this->get_height(); row++) {
    for (int col = 0; col < this->get_width(); col++) {
      RGBColor pix_data_color = pix_data.at(row, col);
      if (!this->at(row, col).equals(pix_data_color)) {
        return false;
      }
    }
  }
  return true;
}

const RGBColor& PixelData::at(int row, int col) const {
  return this->pixels[row * this->m_width + col];
}

PixelData::PixelData(const char* file_name) {
  AVFormatContext* format_context;
  format_context = open_format_context(std::string(file_name));

  std::vector<enum AVMediaType> media_types = { AVMEDIA_TYPE_VIDEO };
  std::vector<std::shared_ptr<StreamData>> media_streams = get_stream_datas(format_context, media_types);

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

  std::vector<AVFrame*> original_frame_container;

  while (av_read_frame(format_context, packet) == 0) {

    if (packet->stream_index != imageStream.get_stream_index()) {
      continue;
    }

    try {
      original_frame_container = decode_video_packet(codec_context, packet);
      final_frame = image_converter.convert_video_frame(original_frame_container[0]);
      clear_av_frame_list(original_frame_container);
    } catch (ascii::ffmpeg_error const& e) {
      if (e.get_averror() == AVERROR(EAGAIN)) {
        continue;
      } else {
        av_packet_free((AVPacket**)&packet);
        av_frame_free((AVFrame**)&final_frame);
        throw ascii::ffmpeg_error("ERROR WHILE READING PACKET DATA FROM IMAGE FILE: " + std::string(file_name), e.get_averror());
      }
    }

  }

  this->pixels.reserve(final_frame->width * final_frame->height * 3);
  this->m_height = final_frame->height;
  this->m_width = final_frame->width;
  for (int i = 0; i < final_frame->width * final_frame->height; i++) {
    this->pixels.push_back(RGBColor( final_frame->data[0][i * 3], final_frame->data[0][i * 3 + 1], final_frame->data[0][i * 3 + 2] ));
  }

  av_packet_free((AVPacket**)&packet);
  av_frame_free((AVFrame**)&final_frame);
  avformat_free_context(format_context);
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
