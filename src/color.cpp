#include "color.h"

#include "wmath.h"
#include "funcmac.h"

#include <fmt/format.h>


#include <cmath>
#include <vector>
#include <stdexcept>
#include <cstdint>

RGBColor RGBColor::BLACK = RGBColor(0, 0, 0);
RGBColor RGBColor::WHITE = RGBColor(255, 255, 255);

double RGBColor::dis_sq(const RGBColor& other) const {
  // credit to https://www.compuphase.com/cmetric.htm 
  long rmean = ( static_cast<long>(this->r) + static_cast<long>(other.r) ) / 2L;
  long r = static_cast<long>(this->r) - static_cast<long>(other.r);
  long g = static_cast<long>(this->g) - static_cast<long>(other.g);
  long b = static_cast<long>(this->b) - static_cast<long>(other.b);
  return (((512L+rmean)*r*r)>>8) + 4*g*g + (((767L-rmean)*b*b)>>8);
}

double RGBColor::distance(const RGBColor& other) const {
  // credit to https://www.compuphase.com/cmetric.htm 
  return std::sqrt(this->dis_sq(other));
}


RGBColor get_average_color(std::vector<RGBColor>& colors) {
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

  return RGBColor((sums[0]/colors.size()) & 0xFF, (sums[1]/colors.size()) & 0xFF, (sums[2]/colors.size()) & 0xFF);
}

std::uint8_t RGBColor::gray_val() const {
  return get_gray8(this->r, this->g, this->b);
}


int get_grayint(int r, int g, int b) {
  return (int)(0.299 * r + 0.587 * g + 0.114 * b);
}

std::uint8_t get_gray8(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return (std::uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
}

RGBColor RGBColor::get_comp() const {
  return RGBColor(255 - this->r, 255 - this->g, 255 - this->b );
} 

bool RGBColor::is_gray() const {
  return this->r == this->g && this->g == this->b;
}


RGBColor RGBColor::as_gray() const {
  std::uint8_t value = get_gray8(this->r, this->g, this->b);
  return RGBColor(value);
}

RGBColor find_closest_color(RGBColor& input, std::vector<RGBColor>& colors) {
  int index = find_closest_color_index(input, colors);
  return colors[index];
}

int find_closest_color_index(RGBColor& input, std::vector<RGBColor>& colors) {
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

void RGBColor::operator=(const RGBColor& color) {
  this->r = color.r;
  this->g = color.g;
  this->b = color.b;
}

void RGBColor::operator=(RGBColor&& color) {
  this->r = color.r;
  this->g = color.g;
  this->b = color.b;
}

bool RGBColor::equals(const RGBColor& other) const {
  return this->r == other.r && this->g == other.g && this->b == other.b;
}

bool RGBColor::operator==(const RGBColor& other) const {
  return this->r == other.r && this->g == other.g && this->b == other.b;
}

bool RGBColor::operator==(RGBColor&& other) const {
  return this->r == other.r && this->g == other.g && this->b == other.b;
}