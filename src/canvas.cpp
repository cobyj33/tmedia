#include "canvas.h"

#include "color.h"
#include "pixeldata.h"

#include <vector>
#include <utility>

Canvas::Canvas(int nb_rows, int nb_cols) {
  this->nb_rows = nb_rows;
  this->nb_cols = nb_cols;
  this->canvas.reserve(nb_rows * nb_cols);
  for (int i = 0; i < nb_rows * nb_cols; i++) {
    this->canvas.push_back(std::move(RGBColor(0)));
  }
}

void Canvas::line(int row1, int col1, int row2, int col2, RGBColor color) {
  if (col1 == col2) {
    for (int row = std::min(row1, row2); row <= std::max(row1, row2); row++) {
        this->canvas[row * this->nb_cols + col1] = color;
    }
  }
  else if (row1 == row2) {
    for (int col = std::min(col1, col2); col <= std::max(col1, col2); col++) {
        this->canvas[row1 * this->nb_cols + col] = color;
    }
  } else {
    double slope = (row1 - row2) / (col1 - col2);
    double yIntercept = row1 - (slope * col1);
    for (int col = std::min(col1, col2); col <= std::max(col1, col2); col++) {
        int row = (slope * col) + yIntercept;
        this->canvas[row * this->nb_cols + col] = color;
    }
    for (int row = std::min(row1, row2); row <= std::max(row1, row2); row++) {
        int col = (row - yIntercept) / slope;
        this->canvas[row * this->nb_cols + col] = color;
    }
  }
}

PixelData Canvas::get_image() {
  return std::move(PixelData(this->canvas, this->nb_cols, this->nb_rows));
}