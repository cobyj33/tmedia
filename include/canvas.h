#ifndef TMEDIA_CANVAS_H
#define TMEDIA_CANVAS_H

#include <vector>
#include <memory>

#include "pixeldata.h"
#include "color.h"


/**
 * A simple mutable fixed-dimension image class which allows drawing geometry to
 * a bitmap and returning an immutable PixelData object with get_image()
 * 
 * Note that only drawing lines is actually implemented as of now, as it is
 * the only routine currently needed by tmedia
*/
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