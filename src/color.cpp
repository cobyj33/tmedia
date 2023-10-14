#include "color.h"

#include "wmath.h"

#include <cmath>
#include <vector>
#include <stdexcept>

RGBColor RGBColor::BLACK = RGBColor(0, 0, 0);
RGBColor RGBColor::WHITE = RGBColor(255, 255, 255);

double RGBColor::distance_squared(const RGBColor& other) const {
  // credit to https://www.compuphase.com/cmetric.htm 
  long rmean = ( (long)this->red + (long)other.red ) / 2;
  long r = (long)this->red - (long)other.red;
  long g = (long)this->green - (long)other.green;
  long b = (long)this->blue - (long)other.blue;
  return (((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8);
}

double RGBColor::distance(const RGBColor& other) const {
  // credit to https://www.compuphase.com/cmetric.htm 
  return std::sqrt(this->distance_squared(other));
}


RGBColor get_average_color(std::vector<RGBColor>& colors) {
  if (colors.size() == 0) {
    throw std::runtime_error("Cannot find average color value of empty vector");
  }

  int sums[3] = {0, 0, 0};
  for (int i = 0; i < (int)colors.size(); i++) {
    sums[0] += (double)colors[i].red * colors[i].red;
    sums[1] += (double)colors[i].green * colors[i].green;
    sums[2] += (double)colors[i].blue * colors[i].blue;
  }

  return RGBColor((int)std::sqrt(sums[0]/colors.size()), (int)std::sqrt(sums[1]/colors.size()), (int)std::sqrt(sums[2]/colors.size()));
}

int RGBColor::get_grayscale_value() const {
  return get_grayscale(this->red, this->green, this->blue);
}


int get_grayscale(int r, int g, int b) {
  return (int)(0.299 * r + 0.587 * g + 0.114 * b);
}

RGBColor RGBColor::get_complementary() const {
  return RGBColor(255 - this->red, 255 - this->green, 255 - this->blue );
} 

bool RGBColor::is_grayscale() const {
  return this->red == this->green && this->green == this->blue;
}


RGBColor RGBColor::create_grayscale() const {
  int value = get_grayscale(this->red, this->green, this->blue);
  return RGBColor(value);
}

bool RGBColor::equals(const RGBColor& other) const {
  return this->red == other.red && this->green == other.green && this->blue == other.blue;
}


RGBColor find_closest_color(RGBColor& input, std::vector<RGBColor>& colors) {
  int index = find_closest_color_index(input, colors);
  return colors[index];
}

int find_closest_color_index(RGBColor& input, std::vector<RGBColor>& colors) {
  if (colors.size() == 0) {
    throw std::runtime_error("Cannot find closest color index, colors list is empty ");
  }

  int best_color = 0;
  double best_distance = (double)INT32_MAX;
  for (int i = 0; i < (int)colors.size(); i++) {
    double distance = colors[i].distance_squared(input);
    if (distance < best_distance) {
      best_color = i;
      best_distance = distance;
    }
  }

  return best_color;
}

void RGBColor::operator=(const RGBColor& color) {
  this->red = color.red;
  this->green = color.green;
  this->blue = color.blue;
}