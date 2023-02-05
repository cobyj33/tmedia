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
double clamp(double value, double bounds1, double bounds2);
long clamp(long value, long bounds1, long bounds2);

bool in_range(int value, int bounds1, int bounds2);
bool in_range(double value, double bounds1, double bounds2);
bool in_range(long value, long bounds1, long bounds2);

int get_next_nearest_perfect_square(double num);
int get_next_nearest_perfect_square(int num);

int digit_char_to_int(char num_char);

#endif
