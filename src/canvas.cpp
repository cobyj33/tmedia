#include "canvas.h"

#include "color.h"
#include "pixeldata.h"

#include <vector>

Canvas::Canvas(int nb_rows, int nb_cols) {
  this->canvas.reserve(nb_rows);
  for (int row = 0; row < nb_rows; row++) {
    std::vector<RGBColor> column;
    column.reserve(nb_cols);
    for (int col = 0; col < nb_cols; col++) {
      column.push_back(RGBColor(0));
    }
    this->canvas.push_back(std::move(column));
  }
  this->nb_cols = nb_cols;
  this->nb_rows = nb_rows;
}

void Canvas::line(int row1, int col1, int row2, int col2, RGBColor color) {
  if (col1 == col2) {
    for (int row = std::min(row1, row2); row <= std::max(row1, row2); row++) {
        this->canvas[row][col1] = color;
    }
  }
  else if (row1 == row2) {
    for (int col = std::min(col1, col2); col <= std::max(col1, col2); col++) {
        this->canvas[row1][col] = color;
    }
  } else {
    double slope = (row1 - row2) / (col1 - col2);
    double yIntercept = row1 - (slope * col1);
    for (int col = std::min(col1, col2); col <= std::max(col1, col2); col++) {
        int row = (slope * col) + yIntercept;
        this->canvas[row][col] = color;
    }
    for (int row = std::min(row1, row2); row <= std::max(row1, row2); row++) {
        int col = (row - yIntercept) / slope;
        this->canvas[row][col] = color;
    }
  }
}

PixelData Canvas::get_image() {
  return PixelData(this->canvas);
}