#include "color.h"

#include "wmath.h"
#include "funcmac.h"

#include <fmt/format.h>


#include <cmath>
#include <vector>
#include <stdexcept>
#include <cstdint>

RGB24 RGB24::BLACK = RGB24(0, 0, 0);
RGB24 RGB24::WHITE = RGB24(255, 255, 255);

double RGB24::dis_sq(const RGB24& other) const {
  // credit to https://www.compuphase.com/cmetric.htm 
  long rmean = ( static_cast<long>(this->r) + static_cast<long>(other.r) ) / 2L;
  long r = static_cast<long>(this->r) - static_cast<long>(other.r);
  long g = static_cast<long>(this->g) - static_cast<long>(other.g);
  long b = static_cast<long>(this->b) - static_cast<long>(other.b);
  return (((512L+rmean)*r*r)>>8) + 4*g*g + (((767L-rmean)*b*b)>>8);
}

double RGB24::distance(const RGB24& other) const {
  // credit to https://www.compuphase.com/cmetric.htm 
  return std::sqrt(this->dis_sq(other));
}


RGB24 get_average_color(std::vector<RGB24>& colors) {
  if (colors.size() == 0) {
    throw std::runtime_error(fmt::format("[{}] Cannot find average color value "
    "of empty vector", FUNCDINFO));
  }

  int sums[3] = {0, 0, 0};
  for (std::size_t i = 0; i < colors.size(); i++) {
    sums[0] += colors[i].r;
    sums[1] += colors[i].g;
    sums[2] += colors[i].b;
  }

  return RGB24((sums[0]/colors.size()) & 0xFF, (sums[1]/colors.size()) & 0xFF, (sums[2]/colors.size()) & 0xFF);
}

std::uint8_t RGB24::gray_val() const {
  return get_gray8(this->r, this->g, this->b);
}


int get_grayint(int r, int g, int b) {
  return (int)(0.299 * r + 0.587 * g + 0.114 * b);
}

std::uint8_t get_gray8(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return (std::uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
}

RGB24 RGB24::get_comp() const {
  return RGB24(255 - this->r, 255 - this->g, 255 - this->b );
} 

bool RGB24::is_gray() const {
  return this->r == this->g && this->g == this->b;
}


RGB24 RGB24::as_gray() const {
  std::uint8_t value = get_gray8(this->r, this->g, this->b);
  return RGB24(value);
}

RGB24 find_closest_color(RGB24& input, std::vector<RGB24>& colors) {
  int index = find_closest_color_index(input, colors);
  return colors[index];
}

int find_closest_color_index(RGB24& input, std::vector<RGB24>& colors) {
  if (colors.size() == 0) {
    throw std::runtime_error(fmt::format("[{}] Cannot find closest color "
    "index, colors list is empty", FUNCDINFO));
  }

  int best_color = 0;
  double best_distance = (double)INT32_MAX;
  for (std::size_t i = 0; i < colors.size(); i++) {
    double distance = colors[i].dis_sq(input);
    if (distance < best_distance) {
      best_color = i;
      best_distance = distance;
    }
  }

  return best_color;
}

void RGB24::operator=(const RGB24& color) {
  this->r = color.r;
  this->g = color.g;
  this->b = color.b;
}

void RGB24::operator=(RGB24&& color) {
  this->r = color.r;
  this->g = color.g;
  this->b = color.b;
}

bool RGB24::equals(const RGB24& other) const {
  return this->r == other.r && this->g == other.g && this->b == other.b;
}

bool RGB24::operator==(const RGB24& other) const {
  return this->r == other.r && this->g == other.g && this->b == other.b;
}

bool RGB24::operator==(RGB24&& other) const {
  return this->r == other.r && this->g == other.g && this->b == other.b;
}