#include <tmedia/image/color.h>

#include <tmedia/util/wmath.h>
#include <tmedia/util/defines.h>

#include <fmt/format.h>

#include <cmath> // for std::sqrt
#include <vector>
#include <stdexcept>
#include <cstdint>

RGB24 RGB24::BLACK = RGB24(0, 0, 0);
RGB24 RGB24::WHITE = RGB24(255, 255, 255);

double RGB24::dis_sq(RGB24 other) const {
  // credit to https://www.compuphase.com/cmetric.htm 
  long rmean = ( static_cast<long>(this->r) + static_cast<long>(other.r) ) / 2L;
  long r = static_cast<long>(this->r) - static_cast<long>(other.r);
  long g = static_cast<long>(this->g) - static_cast<long>(other.g);
  long b = static_cast<long>(this->b) - static_cast<long>(other.b);
  return (((512L+rmean)*r*r)>>8) + 4*g*g + (((767L-rmean)*b*b)>>8);
}

double RGB24::distance(RGB24 other) const {
  // credit to https://www.compuphase.com/cmetric.htm 
  return std::sqrt(this->dis_sq(other));
}


RGB24 get_average_color(std::vector<RGB24>& colors) {
  int sums[3] = {0, 0, 0};
  for (std::size_t i = 0; i < colors.size(); i++) {
    sums[0] += colors[i].r;
    sums[1] += colors[i].g;
    sums[2] += colors[i].b;
  }

  return RGB24((sums[0]/colors.size()) & 0xFF, (sums[1]/colors.size()) & 0xFF, (sums[2]/colors.size()) & 0xFF);
}


RGB24 find_closest_color(RGB24 input, std::vector<RGB24>& colors) {
  int index = find_closest_color_index(input, colors);
  return colors[index];
}

int find_closest_color_index(RGB24 input, std::vector<RGB24>& colors) {
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


