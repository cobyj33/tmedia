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

/**
 * @param width width must be greater than or equal to zero
 * @param height height must be greater than or equal to zero
*/
void pixdata_setnewdims(PixelData& dest, int width, int height);

/**
 * @param width width must be greater than or equal to zero
 * @param height height must be greater than or equal to zero
 * @param g The grayscale value to initialize all pixels in the PixelData to
*/
void pixdata_initgray(PixelData& dest, int width, int height, std::uint8_t g);


/**
 * 
 * 
 */
void pixdata_copy(PixelData& dest, PixelData& src);

/**
 * 
 * @param dest The destination PixelData to copy the frame data from src into 
* @param src src cannot be null 
*/
void pixdata_from_avframe(PixelData& dest, AVFrame* src);

void vertline(PixelData& dest, int row1, int row2, int col, const RGB24 color);

void horzline(PixelData& dest, int col1, int col2, int row, const RGB24 color);

#endif
