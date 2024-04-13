#ifndef TMEDIA_CANVAS_H
#define TMEDIA_CANVAS_H

#include <vector>
#include <memory>

class PixelData;

#include <tmedia/image/color.h>


/**
 * A simple mutable fixed-dimension image class which allows drawing geometry to
 * a bitmap and returning an immutable PixelData object with get_image()
 * 
 * Note that only drawing lines is actually implemented as of now, as it is
 * the only routine currently needed by tmedia
*/
class Canvas {
  private:
    std::shared_ptr<std::vector<RGB24>> canvas;
    int nb_rows;
    int nb_cols;

  public:
    Canvas(int rows, int cols);
    void line(int row1, int col1, int row2, int col2, const RGB24 color);
    void vertline(int row1, int row2, int col, const RGB24 color);
    void horzline(int col1, int col2, int row, const RGB24 color);
    PixelData get_image();
};

#endif