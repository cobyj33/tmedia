#include <tmedia/image/color.h>

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
