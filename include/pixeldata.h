#ifndef TMEDIA_PIXEL_DATA_H
#define TMEDIA_PIXEL_DATA_H
/**
 * @file audio.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Common functions for audio manipulation in tmedia
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 */

#include "color.h"

#include <cstdint>
#include <vector>
#include <memory>

extern "C" {
#include <libavutil/frame.h>
}

/**
 * Different Scaling Algorithms used in 
*/
enum class ScalingAlgo {
  BOX_SAMPLING,
  NEAREST_NEIGHBOR
};



typedef RGB24 FillFunction(int row, int col);

/**
 * An immutable raw image bitmap class to handle transferring and storing image
 * data portably.
 * 
 * PixelData's immutability allows copying from one PixelData instance to another
 * to be extremely cheap, as PixelData's only need to copy a pointer
 * to the internal image data instead of copying every pixel in the bitmap.
 * The internal image data will be uninitialized once the final reference to
 * the given internal bitmap is erased.
*/
class PixelData {
  private:
    std::shared_ptr<std::vector<RGB24>> pixels;
    int m_width;
    int m_height;

    void init_from_avframe(AVFrame* video_frame);
  public:

    PixelData() : pixels(std::make_shared<std::vector<RGB24>>()), m_width(0), m_height(0) {}
    PixelData(const std::vector< std::vector<RGB24> >& raw_rgb_data);
    PixelData(const std::vector< std::vector<uint8_t> >& raw_grayscale_data);
    PixelData(const std::vector<RGB24>& colors, int width, int height);
    PixelData(std::shared_ptr<std::vector<RGB24>> colors, int width, int height);
    PixelData(int width, int height);
    PixelData(AVFrame* video_frame);
    PixelData(const PixelData& pix_data);
    PixelData(PixelData&& pix_data);

    PixelData scale(double amount, ScalingAlgo scaling_algorithm) const;
    PixelData bound(int width, int height, ScalingAlgo scaling_algorithm) const;
    
    void operator=(const PixelData& pix_data);
    void operator=(PixelData&& pix_data);
    void operator=(AVFrame* video_frame);

    bool equals(const PixelData& pix_data) const;

    inline int get_width() const {
      return this->m_width;
    }

    inline int get_height() const {
      return this->m_height;
    }

    inline bool in_bounds(int row, int col) const {
      return row >= 0 && col >= 0 && row < this->m_height && col < this->m_width;
    }

    inline const RGB24& at(int row, int col) const {
      return (*this->pixels)[row * this->m_width + col];
    }

    inline const std::vector<RGB24>& data() const {
      return (*this->pixels);
    }
};

RGB24 get_avg_color_from_area(const PixelData& data, int row, int col, int width, int height);

inline RGB24 get_avg_color_from_area(const PixelData& pixel_data, double row, double col, double width, double height) {
  return get_avg_color_from_area(pixel_data, static_cast<int>(std::floor(row)),
        static_cast<int>(std::floor(col)), static_cast<int>(std::ceil(width)), static_cast<int>(std::ceil(height)));
}

#endif
