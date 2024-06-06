#ifndef TMEDIA_W_MATH_H
#define TMEDIA_W_MATH_H

#include <algorithm>

[[gnu::always_inline]] constexpr inline int clamp(int val, int b1, int b2) {
  int lower = std::min(b1, b2);
  int upper = std::max(b1, b2);
  return std::max(lower, std::min(upper, val));
}

[[gnu::always_inline]] constexpr inline double clamp(double val, double b1, double b2) {
  double lower = std::min(b1, b2);
  double upper = std::max(b1, b2);
  return std::max(lower, std::min(upper, val));
}

[[gnu::always_inline]] constexpr inline long clamp(long val, long b1, long b2) {
  long lower = std::min(b1, b2);
  long upper = std::max(b1, b2);
  return std::max(lower, std::min(upper, val));
}

[[gnu::always_inline]] constexpr inline bool in_range(int val, int b1, int b2) {
  return std::min(b1, b2) <= val && std::max(b1, b2) >= val;
}

[[gnu::always_inline]] constexpr inline bool in_range(double val, double b1, double b2) {
  return std::min(b1, b2) <= val && std::max(b1, b2) >= val;
}

[[gnu::always_inline]] constexpr inline bool in_range(long val, long b1, long b2) {
  return std::min(b1, b2) <= val && std::max(b1, b2) >= val;
}

[[gnu::always_inline]] constexpr inline int digit_char_to_int(char num_char) {
  return (num_char >= '0' && num_char <= '9') * ((int)num_char - 48);
}

#endif
