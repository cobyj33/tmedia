#ifndef TMEDIA_W_MATH_H
#define TMEDIA_W_MATH_H

inline int signum(int num) {
  return (num > 0) * -(num < 0);
}

inline int signum(double num) {
  return (num > 0) * -(num < 0);
}

inline int min(int b1, int b2) {
  return (b1 < b2) * b1 + (b2 <= b1) * b2; 
}

inline float min(float b1, float b2) {
  return (b1 < b2) * b1 + (b2 <= b1) * b2; 
}

inline long min(long b1, long b2) {
  return (b1 < b2) * b1 + (b2 <= b1) * b2; 
}

inline double min(double b1, double b2) {
  return (b1 < b2) * b1 + (b2 <= b1) * b2; 
}

inline int max(int b1, int b2) {
  return (b1 > b2) * b1 + (b2 >= b1) * b2; 
}

inline float max(float b1, float b2) {
  return (b1 > b2) * b1 + (b2 >= b1) * b2; 
}

inline long max(long b1, long b2) {
  return (b1 > b2) * b1 + (b2 >= b1) * b2; 
}

inline double max(double b1, double b2) {
  return (b1 > b2) * b1 + (b2 >= b1) * b2; 
}

inline int clamp(int val, int b1, int b2) {
  int lower = min(b1, b2);
  int upper = max(b1, b2);
  return max(lower, min(upper, val));
}

inline double clamp(double val, double b1, double b2) {
  double lower = min(b1, b2);
  double upper = max(b1, b2);
  return max(lower, min(upper, val));
}

inline long clamp(long val, long b1, long b2) {
  long lower = min(b1, b2);
  long upper = max(b1, b2);
  return max(lower, min(upper, val));
}

inline bool in_range(int val, int b1, int b2) {
  return min(b1, b2) <= val && max(b1, b2) >= val;
}

inline bool in_range(double val, double b1, double b2) {
  return min(b1, b2) <= val && max(b1, b2) >= val;
}

inline bool in_range(long val, long b1, long b2) {
  return min(b1, b2) <= val && max(b1, b2) >= val;
}

inline int digit_char_to_int(char num_char) {
  return (num_char >= '0' && num_char <= '9') * ((int)num_char - 48);
}

#endif
