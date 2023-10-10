#ifndef ASCII_VIDEO_CANVAS_H
#define ASCII_VIDEO_CANVAS_H

#include <vector>
#include "pixeldata.h"
#include "color.h"

class Canvas {
  private:
    std::vector<std::vector<RGBColor>> canvas;

  public:
    Canvas(int width, int height);
    void line(int row1, int col1, int row2, int col2, RGBColor color);
    PixelData get_image();
};

#endif