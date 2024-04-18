#include <tmedia/image/pixeldata.h>

#include <tmedia/image/color.h>
#include <tmedia/util/wmath.h>
#include <tmedia/image/scale.h>
#include <tmedia/ffmpeg/decode.h>
#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/ffmpeg/videoconverter.h>
#include <tmedia/ffmpeg/ffmpeg_error.h>
#include <tmedia/ffmpeg/streamdecoder.h>

#include <tmedia/util/defines.h>

#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <functional>
#include <utility>
#include <memory>

#include <cassert>

#include <fmt/format.h>

extern "C" {
  #include <libavutil/frame.h>
}

// Accepts empty matrices
template <typename T>
bool is_rect_vec_mat(const std::vector<std::vector<T>>& vector_2d) {
  if (vector_2d.size() == 0)
    return true;

  std::size_t width = vector_2d[0].size();
  for (int row = 0; row < (int)vector_2d.size(); row++)
    if (vector_2d[row].size() != width)
      return false;

  return true;
}

// remember to allocate this->pixels before calling init_from_avframe
void PixelData::init_from_avframe(AVFrame* video_frame) {
  this->pixels->clear();
  this->pixels->reserve(video_frame->width * video_frame->height);
  this->m_width = video_frame->width;
  this->m_height = video_frame->height;
  const uint8_t* const data = video_frame->data[0];
  std::vector<RGB24>& pixels = *this->pixels;

  switch (static_cast<AVPixelFormat>(video_frame->format)) {
    case AV_PIX_FMT_GRAY8: {
      const int datalen = this->m_width * this->m_height;
      for (int i = 0; i < datalen; i++) {
        pixels[i] = RGB24(data[i]);
      }
    } break;
    case AV_PIX_FMT_RGB24: {
      const int area = this->m_width * this->m_height;
      int di = 0;
      for (int i = 0; i < area; i++) {
        pixels[i] = RGB24(data[di], data[di + 1], data[di + 2]);
        di += 3;
      }
    } break;
    default: throw std::runtime_error(fmt::format("[{}] Passed in AVFrame "
    "with unimplemeted format, only supported formats for initializing from "
    "AVFrame are AV_PIX_FMT_GRAY8 and AV_PIX_FMT_RGB24", FUNCDINFO));
  }
}

PixelData::PixelData(const std::vector<std::vector<RGB24>>& rgbm) {
  if (!is_rect_vec_mat(rgbm)) {
    throw std::runtime_error(fmt::format("[{}] Cannot initialize pixel data "
                                    "with non-rectangular matrix", FUNCDINFO));
  }

  this->pixels = std::make_shared<std::vector<RGB24>>();
  this->m_height = rgbm.size();
  this->m_width = rgbm.size() > 0 ? rgbm[0].size() : 0;
  this->pixels->reserve(this->m_height * this->m_width);

  for (int row = 0; row < this->m_height; row++) {
    for (int col = 0; col < this->m_width; col++) {
      (*this->pixels)[row * this->m_width + col] = rgbm[row][col];
    }
  }
}

PixelData::PixelData(const std::vector<std::vector<uint8_t> >& graym) {
  if (!is_rect_vec_mat(graym)) {
    throw std::runtime_error(fmt::format("[{}]Cannot initialize pixel data "
                                    "with non-rectangular matrix", FUNCDINFO));
  }

  this->pixels = std::make_shared<std::vector<RGB24>>();
  this->m_height = graym.size();
  this->m_width = graym.size() > 0 ? graym[0].size() : 0;
  this->pixels->reserve(this->m_height * this->m_width);

  for (int row = 0; row < this->m_height; row++) {
    for (int col = 0; col < this->m_width; col++) {
      (*this->pixels)[row * this->m_width + col] = RGB24(graym[row][col]);
    }
  }
}

PixelData::PixelData(const std::vector<RGB24>& flatrgb, int width, int height) {
  if (std::size_t(width * height) != flatrgb.size()) 
    throw std::runtime_error(fmt::format("[{}] Cannot initialize PixelData "
    "with innacurate flattened rgb vector: size = {}, given width: {}, "
    "given height: {}", FUNCDINFO, flatrgb.size(), width, height));

  this->pixels = std::make_shared<std::vector<RGB24>>();
  this->pixels->reserve(width * height);
  this->m_width = width;
  this->m_height = height;
  const int area = width * height; 

  for (int i = 0; i < area; i++) {
    (*this->pixels)[i] = flatrgb[i];
  }
}

PixelData::PixelData(std::shared_ptr<std::vector<RGB24>> colors, int width, int height) {
  if (std::size_t(width * height) != colors->size()) 
    throw std::runtime_error(fmt::format("[{}] Cannot initialize PixelData "
    "with innacurate flattened rgb vector: size = {}, given width: {}, "
    "given height: {}", FUNCDINFO, colors->size(), width, height));

  this->pixels = colors;
  this->m_width = width;
  this->m_height = height;
}

PixelData::PixelData(AVFrame* video_frame) {
  this->pixels = std::make_shared<std::vector<RGB24>>();
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
  this->pixels = std::move(pix_data.pixels);
  this->m_width = pix_data.m_width;
  this->m_height = pix_data.m_height;
}

void PixelData::operator=(const PixelData& pix_data) {
  this->pixels = pix_data.pixels;
  this->m_width = pix_data.m_width;
  this->m_height = pix_data.m_height;
}

void PixelData::operator=(PixelData&& pix_data) {
  this->pixels = std::move(pix_data.pixels);
  this->m_width = pix_data.m_width;
  this->m_height = pix_data.m_height;
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
  std::shared_ptr<std::vector<RGB24>> new_pixels = std::make_shared<std::vector<RGB24>>();
  new_pixels->reserve(new_width * new_height);

  switch (scaling_algorithm) {
    case ScalingAlgo::BOX_SAMPLING: {
      double box_width = 1 / amount;
      double box_height = 1 / amount;

      for (double new_row = 0; new_row < new_height; new_row++) {
        for (double new_col = 0; new_col < new_width; new_col++) {
          new_pixels->push_back(get_avg_color_from_area(*this, new_row * box_height, new_col * box_width, box_width, box_height ));
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
      RGB24 pix_data_color = pix_data.at(row, col);
      if (!this->at(row, col).equals(pix_data_color)) {
        return false;
      }
    }
  }
  return true;
}


RGB24 get_avg_color_from_area(const PixelData& pixel_data, int row, int col, int width, int height) {
  assert(width * height > 0);
  assert(row >= 0);
  assert(col >= 0);

  const int rowmax = std::min(pixel_data.get_height(), row + height);
  const int colmax = std::min(pixel_data.get_width(), col + width);
  int sums[3] = {0, 0, 0};
  int colors = 0;

  for (int curr_row = row; curr_row < rowmax; curr_row++) {
    for (int curr_col = col; curr_col < colmax; curr_col++) {
      RGB24 color = pixel_data.at(curr_row, curr_col);
      sums[0] += color.r;
      sums[1] += color.g;
      sums[2] += color.b;
      colors++;
    }
  }

  colors = (colors == 0) ? 1 : colors;
  return RGB24((sums[0]/colors) & 0xFF, (sums[1]/colors) & 0xFF, (sums[2]/colors) & 0xFF);
}
