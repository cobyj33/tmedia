#ifndef TMEDIA_W_MATH_H
#define TMEDIA_W_MATH_H

#include <algorithm>

template <typename T>
[[gnu::always_inline]] constexpr inline int clamp(T val, T b1, T b2) {
  T lower = std::min(b1, b2);
  T upper = std::max(b1, b2);
  return std::max(lower, std::min(upper, val));
}

template <typename T>
[[gnu::always_inline]] constexpr inline bool in_range(T val, T b1, T b2) {
  return std::min(b1, b2) <= val && std::max(b1, b2) >= val;
}

[[gnu::always_inline]] constexpr inline int digit_char_to_int(char num_char) {
  return (num_char >= '0' && num_char <= '9') * ((int)num_char - 48);
}

#endif
