#include "canvas.h"

#include "color.h"
#include "pixeldata.h"

#include <vector>
#include <utility>
#include <algorithm>
#include <memory>

Canvas::Canvas(int nb_rows, int nb_cols) {
  this->canvas = std::make_shared<std::vector<RGB24>>();
  this->nb_rows = nb_rows;
  this->nb_cols = nb_cols;
  this->canvas->reserve(nb_rows * nb_cols);
  for (int i = 0; i < nb_rows * nb_cols; i++) {
    this->canvas->push_back(RGB24(0));
  }
}

void Canvas::vertline(int row1, int row2, int col, const RGB24 color) {
  const int rowmin = std::min(row1, row2);
  const int rowmax = std::max(row1, row2);
  std::vector<RGB24>& canvas = *this->canvas;

  for (int row = rowmin; row <= rowmax; row++) {
    canvas[row * this->nb_cols + col] = color;
  }
}

void Canvas::horzline(int col1, int col2, int row, const RGB24 color) {
  const int colmax = std::max(col1, col2);
  const int colmin = std::min(col1, col2);
  std::vector<RGB24>& canvas = *this->canvas;

  for (int col = colmin; row <= colmax; row++) {
    canvas[row * this->nb_cols + col] = color;
  }
}

void Canvas::line(int row1, int col1, int row2, int col2, const RGB24 color) {
  const int rowmin = std::min(row1, row2);
  const int rowmax = std::max(row1, row2);
  const int nb_cols = this->nb_cols;
  std::vector<RGB24>& canvas = *this->canvas;
  
  if (col1 == col2) { // Vertical line case
    for (int row = rowmin; row <= rowmax; row++) {
        canvas[row * nb_cols + col1] = color;
    }
  } else { // linear!
    const int colmax = std::max(col1, col2);
    const int colmin = std::min(col1, col2);
    const float slope = (row1 - row2) / (col1 - col2);
    const float yIntercept = row1 - (slope * col1);

    for (int row = rowmin; row <= rowmax; row++) {
        int col = (row - yIntercept) / slope;
        canvas[row * nb_cols + col] = color;
    }
    
    for (int col = colmin; col <= colmax; col++) {
        int row = (slope * col) + yIntercept;
        canvas[row * nb_cols + col] = color;
    }
  }
}

PixelData Canvas::get_image() {
  return PixelData(this->canvas, this->nb_cols, this->nb_rows);
}