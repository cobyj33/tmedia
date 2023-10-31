#ifndef TMEDIA_CANVAS_H
#define TMEDIA_CANVAS_H

#include <vector>
#include <memory>

#include "pixeldata.h"
#include "color.h"

class Canvas {
  private:
    std::shared_ptr<std::vector<RGBColor>> canvas;
    int nb_rows;
    int nb_cols;

  public:
    Canvas(int rows, int cols);
    void line(int row1, int col1, int row2, int col2, RGBColor color);
    PixelData get_image();
};

#endif