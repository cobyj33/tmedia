#ifndef ASCII_VIDEO_PIXEL_DATA
#define ASCII_VIDEO_PIXEL_DATA

#include <cstdint>
#include <vector>

#include "color.h"

extern "C" {
#include <libavutil/frame.h>
}

typedef RGBColor FillFunction(int row, int col);

class PixelData {
  private:
    std::vector<RGBColor> pixels;
    int m_width;
    int m_height;

    template <typename T>
    void init_from_source(int width, int height, T&& fill);
    void init_from_avframe(AVFrame* video_frame);
  public:

    PixelData() : pixels(std::vector<RGBColor>()), m_width(0), m_height(0) {}
    PixelData(std::vector< std::vector<RGBColor> >& raw_rgb_data);
    PixelData(std::vector< std::vector<uint8_t> >& raw_grayscale_data);
    // PixelData(std::vector< std::vector<RGBColor> >& raw_rgb_data, int width, int height);
    // PixelData(std::vector<uint8_t>& raw_grayscale_data, int width, int height);
    PixelData(int width, int height);
    PixelData(AVFrame* video_frame);
    PixelData(const char* file_name);
    PixelData(const PixelData& pix_data);
    
    void operator=(const PixelData& pix_data);
    void operator=(AVFrame* video_frame);

    bool equals(const PixelData& pix_data) const;
    int get_width() const;
    int get_height() const;

    PixelData scale(double amount) const;
    PixelData bound(int width, int height) const;

    const RGBColor& at(int row, int column) const;
    bool in_bounds(int row, int column) const;
    RGBColor get_avg_color_from_area(int row, int col, int width, int height) const;
    RGBColor get_avg_color_from_area(double row, double col, double width, double height) const;
};

#endif
