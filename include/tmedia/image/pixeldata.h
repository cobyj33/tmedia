#ifndef TMEDIA_PIXEL_DATA_H
#define TMEDIA_PIXEL_DATA_H

#include <tmedia/image/color.h>

#include <vector>
#include <cstdint>

extern "C" {
struct AVFrame;
}

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
