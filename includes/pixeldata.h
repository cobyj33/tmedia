#ifndef ASCII_VIDEO_PIXEL_DATA
#define ASCII_VIDEO_PIXEL_DATA

#include <cstdint>
#include <vector>
#include "color.h"

extern "C" {
#include <libavutil/frame.h>
}

class PixelData {
    private:
        std::vector< std::vector<RGBColor> > pixels;
    public:

        PixelData() : pixels(std::vector< std::vector<RGBColor> >()) {}
        PixelData(std::vector< std::vector<RGBColor> >& rawData);
        PixelData(std::vector< std::vector<uint8_t> >& rawGrayscaleData);
        PixelData(int width, int height);
        PixelData(AVFrame* videoFrame);
        PixelData(const PixelData& other);
        PixelData(const char* fileName);

        bool equals(const PixelData& other) const;

        int get_width() const;
        int get_height() const;

        PixelData scale(double amount) const;
        PixelData bound(int width, int height) const;

        RGBColor at(int row, int column) const;
        bool in_bounds(int row, int column) const;
        RGBColor atArea(int row, int column, int width, int height) const;
        RGBColor get_avg_color_from_area(int row, int col, int width, int height) const;
        RGBColor get_avg_color_from_area(double row, double col, double width, double height) const;
        
};

#endif
