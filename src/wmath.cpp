#include "wmath.h"

#include "funcmac.h"

#include <cmath>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

int signum(int num) {
  if (num < 0) return -1;
  if (num > 0) return 1;
  return 0;
}

int signum(double num) {
  if (num < 0.0) return -1;
  if (num > 0.0) return 1;
  return 0;
}

int clamp(int val, int b1, int b2) {
  int lower = std::min(b1, b2);
  int upper = std::max(b1, b2);
  return std::max(lower, std::min(upper, val));
}

double clamp(double val, double b1, double b2) {
  double lower = std::min(b1, b2);
  double upper = std::max(b1, b2);
  return std::max(lower, std::min(upper, val));
}

long clamp(long val, long b1, long b2) {
  long lower = std::min(b1, b2);
  long upper = std::max(b1, b2);
  return std::max(lower, std::min(upper, val));
}

bool in_range(int val, int b1, int b2) {
  return std::min(b1, b2) <= val && std::max(b1, b2) >= val;
}

bool in_range(double val, double b1, double b2) {
  return std::min(b1, b2) <= val && std::max(b1, b2) >= val;
}

bool in_range(long val, long b1, long b2) {
  return std::min(b1, b2) <= val && std::max(b1, b2) >= val;
}

int get_next_nearest_perfect_square(double num) {
  double nsqrt = std::sqrt(num);
  if (nsqrt == (int)nsqrt) {
    return std::pow(std::ceil(std::sqrt(num) + 1), 2);
  }

  return std::pow(std::ceil(std::sqrt(num)), 2);
}

int get_next_nearest_perfect_square(int num) {
  double nsqrt = std::sqrt(num);
  if (nsqrt == (int)nsqrt) {
    return std::pow(std::ceil(std::sqrt(num) + 1), 2);
  }

  return std::pow(std::ceil(std::sqrt(num)), 2);
}

int digit_char_to_int(char num_char) {
  if (num_char >= '0' && num_char <= '9') {
    return (int)num_char - 48;
  }
  throw std::runtime_error(fmt::format("[{}] Attempted to convert non-digit "
  "character to number: {}", FUNCDINFO, num_char));
}