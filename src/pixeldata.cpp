#include "pixeldata.h"

#include "color.h"
#include "wmath.h"
#include "scale.h"
#include "decode.h"
#include "boiler.h"
#include "videoconverter.h"
#include "ffmpeg_error.h"
#include "streamdecoder.h"

#include "funcmac.h"

#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <functional>
#include <utility>
#include <memory>

#include <fmt/format.h>

extern "C" {
  #include <libavutil/frame.h>
}

// Accepts empty matrices
template <typename T>
bool is_rectangular_vector_matrix(const std::vector<std::vector<T>>& vector_2d) {
  if (vector_2d.size() == 0)
    return true;

  std::size_t width = vector_2d[0].size();
  for (int row = 0; row < (int)vector_2d.size(); row++)
    if (vector_2d[row].size() != width)
      return false;

  return true;
}

template <typename T>
void PixelData::init_from_source(int width, int height, T&& fill) {
  this->pixels->clear();
  this->pixels->reserve(width * height);
  this->m_width = width;
  this->m_height = height;

  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      this->pixels->push_back(fill(row, col));
    }
  }
}

void PixelData::init_from_avframe(AVFrame* video_frame) {
  this->init_from_source(video_frame->width, video_frame->height,
  [video_frame](int row, int col) {
    switch ((AVPixelFormat)video_frame->format) {
      case AV_PIX_FMT_GRAY8: return RGBColor(video_frame->data[0][row * video_frame->width + col]);
      case AV_PIX_FMT_RGB24: return RGBColor( video_frame->data[0][row * video_frame->width * 3 + col * 3],
      video_frame->data[0][row * video_frame->width * 3 + col * 3 + 1],
      video_frame->data[0][row * video_frame->width * 3 + col * 3 + 2] );
      default: throw std::runtime_error(fmt::format("[{}] Passed in AVFrame "
      "with unimplemeted format, only supported formats for initializing from "
      "AVFrame are AV_PIX_FMT_GRAY8 and AV_PIX_FMT_RGB24", FUNCDINFO));
    }
  });
}

PixelData::PixelData(const std::vector<std::vector<RGBColor>>& raw_rgb_data) {
  if (!is_rectangular_vector_matrix(raw_rgb_data)) {
    throw std::runtime_error(fmt::format("[{}] Cannot initialize pixel data "
                                    "with non-rectangular matrix", FUNCDINFO));
  }

  this->pixels = std::make_shared<std::vector<RGBColor>>();
  this->m_height = raw_rgb_data.size();
  this->m_width = 0;
  if (raw_rgb_data.size() > 0) {
    this->init_from_source((int)raw_rgb_data[0].size(), (int)raw_rgb_data.size(), [raw_rgb_data](int row, int col) { return raw_rgb_data[row][col]; });
  }
}

PixelData::PixelData(const std::vector<std::vector<uint8_t> >& raw_grayscale_data) {
  if (!is_rectangular_vector_matrix(raw_grayscale_data)) {
    throw std::runtime_error(fmt::format("[{}]Cannot initialize pixel data "
                                    "with non-rectangular matrix", FUNCDINFO));
  }
  this->pixels = std::make_shared<std::vector<RGBColor>>();
  this->m_height = raw_grayscale_data.size();
  this->m_width = 0;
  if (raw_grayscale_data.size() > 0) {
    this->init_from_source(raw_grayscale_data[0].size(), raw_grayscale_data.size(), [raw_grayscale_data](int row, int col) { return RGBColor(raw_grayscale_data[row][col]);  });
  }
}

PixelData::PixelData(const std::vector<RGBColor>& flat_rgb, int width, int height) {
  if (std::size_t(width * height) != flat_rgb.size()) 
    throw std::runtime_error(fmt::format("[{}] Cannot initialize PixelData "
    "with innacurate flattened rgb vector: size = {}, given width: {}, "
    "given height: {}", FUNCDINFO, flat_rgb.size(), width, height));

  this->pixels = std::make_shared<std::vector<RGBColor>>();
  this->init_from_source(width, height, [flat_rgb, width](int row, int col) { return flat_rgb[row * width + col]; } );
}

PixelData::PixelData(std::shared_ptr<std::vector<RGBColor>> colors, int width, int height) {
  if (std::size_t(width * height) != colors->size()) 
    throw std::runtime_error(fmt::format("[{}] Cannot initialize PixelData "
    "with innacurate flattened rgb vector: size = {}, given width: {}, "
    "given height: {}", FUNCDINFO, colors->size(), width, height));

  this->pixels = colors;
  this->m_width = width;
  this->m_height = height;
}

PixelData::PixelData(AVFrame* video_frame) {
  this->pixels = std::make_shared<std::vector<RGBColor>>();
  this->init_from_avframe(video_frame);
}

void PixelData::operator=(AVFrame* video_frame) {
  this->init_from_avframe(video_frame);
}

PixelData::PixelData(const PixelData& pix_data) {
  this->pixels = pix_data.pixels;
  this->m_width = pix_data.m_width;
  this->m_height = pix_data.m_height;
}

PixelData::PixelData(PixelData&& pix_data) {
  this->pixels = pix_data.pixels;
  this->m_width = pix_data.m_width;
  this->m_height = pix_data.m_height;
}

void PixelData::operator=(const PixelData& pix_data) {
  this->pixels = pix_data.pixels;
  this->m_width = pix_data.m_width;
  this->m_height = pix_data.m_height;
}

void PixelData::operator=(PixelData&& pix_data) {
  this->pixels = pix_data.pixels;
  this->m_width = pix_data.m_width;
  this->m_height = pix_data.m_height;
}

bool PixelData::in_bounds(int row, int col) const {
  return row >= 0 && col >= 0 && row < this->m_height && col < this->m_width;
}

int PixelData::get_width() const {
  return this->m_width;
}

int PixelData::get_height() const {
  return this->m_height;
}

PixelData PixelData::scale(double amount, ScalingAlgo scaling_algorithm) const {
  if (amount == 0) {
    return PixelData();
  } else if (amount < 0) {
    throw std::runtime_error(fmt::format("[{}] Scaling Pixel data by negative "
    "amount is currently not supported", FUNCDINFO));
  }

  const int new_width = this->get_width() * amount;
  const int new_height = this->get_height() * amount;
  std::shared_ptr<std::vector<RGBColor>> new_pixels = std::make_shared<std::vector<RGBColor>>();
  new_pixels->reserve(new_width * new_height);

  switch (scaling_algorithm) {
    case ScalingAlgo::BOX_SAMPLING: {
      double box_width = 1 / amount;
      double box_height = 1 / amount;

      for (double new_row = 0; new_row < new_height; new_row++) {
        for (double new_col = 0; new_col < new_width; new_col++) {
          new_pixels->push_back(std::move(get_avg_color_from_area(*this, new_row * box_height, new_col * box_width, box_width, box_height )));
        }
      }
    } break;
    case ScalingAlgo::NEAREST_NEIGHBOR: {
      for (double new_row = 0; new_row < new_height; new_row++) {
        for (double new_col = 0; new_col < new_width; new_col++) {
          new_pixels->push_back((*this->pixels)[(int)(new_row / amount) * this->m_width + (int)(new_col / amount)]);
        }
      }
    } break;
    default: throw std::runtime_error(fmt::format("[{}] unrecognized scaling "
    "function", FUNCDINFO));
  }

  return PixelData(new_pixels, new_width, new_height);
}

PixelData PixelData::bound(int width, int height, ScalingAlgo scaling_algorithm) const {
  if (this->get_width() <= width && this->get_height() <= height) {
    return PixelData(*this);
  }

  double scale_factor = get_scale_factor(this->get_width(), this->get_height(), width, height);
  return this->scale(scale_factor, scaling_algorithm);
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
  return (*this->pixels)[row * this->m_width + col];
}

RGBColor get_avg_color_from_area(const PixelData& pixel_data, double row, double col, double width, double height) {
  return get_avg_color_from_area(pixel_data, (int)std::floor(row), (int)std::floor(col), (int)std::ceil(width), (int)std::ceil(height));
}

RGBColor get_avg_color_from_area(const PixelData& pixel_data, int row, int col, int width, int height) {
  if (width * height <= 0) {
    throw std::runtime_error(fmt::format("[{}] Cannot get average color from "
    "an area with dimensions: ( width: {} height: {} ). Dimensions must be "
    "positive", FUNCDINFO, width, height));
  }

  std::vector<RGBColor> colors;
  for (int curr_row = row; curr_row < row + height; curr_row++) {
    for (int curr_col = col; curr_col < col + width; curr_col++) {
      if (pixel_data.in_bounds(curr_row, curr_col)) {
        colors.push_back(pixel_data.at(curr_row, curr_col));
      }
    }
  }

  if (colors.size() > 0) {
    return get_average_color(colors);
  }
  return RGBColor::WHITE;
}
