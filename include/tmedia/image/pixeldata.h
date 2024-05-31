#ifndef TMEDIA_PIXEL_DATA_H
#define TMEDIA_PIXEL_DATA_H
/**
 * @file tmedia/audio/audio.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief Common functions for audio manipulation in tmedia
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 */

#include <tmedia/image/color.h>

#include <vector>
#include <cstdint>

extern "C" {
#include <libavutil/frame.h>
}

typedef RGB24 FillFunction(int row, int col);

class PixelData {
  public:
    std::vector<RGB24> pixels;
    int m_width;
    int m_height;
};

void pixdata_setnewdims(PixelData& dest, int width, int height);
void pixdata_initgray(PixelData& dest, int width, int height, std::uint8_t g);
void pixdata_copy(PixelData& dest, PixelData& src);
void pixdata_from_avframe(PixelData& dest, AVFrame* src);
void vertline(PixelData& dest, int row1, int row2, int col, const RGB24 color);
void horzline(PixelData& dest, int col1, int col2, int row, const RGB24 color);

#endif
