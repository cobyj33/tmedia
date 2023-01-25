#ifndef ASCII_VIDEO_WRAPPER_MATH
#define ASCII_VIDEO_WRAPPER_MATH

#include <cmath>
#include <algorithm>


int signum(int num);
int signum(double num);

/**
 * @brief return a random double between zero and 1
 */
double frand();

int clamp(int value, int bounds1, int bounds2);
double clamp(double value, double lower, double bounds2);
long clamp(long value, long lower, long bounds2);

#endif
