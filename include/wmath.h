#ifndef TMEDIA_W_MATH_H
#define TMEDIA_W_MATH_H


int signum(int num);
int signum(double num);

int clamp(int value, int bounds_1, int bounds_2);
double clamp(double value, double bounds_1, double bounds_2);
long clamp(long value, long bounds_1, long bounds_2);

bool in_range(int value, int bounds_1, int bounds_2);
bool in_range(double value, double bounds_1, double bounds_2);
bool in_range(long value, long bounds_1, long bounds_2);

int get_next_nearest_perfect_square(double num);
int get_next_nearest_perfect_square(int num);

int digit_char_to_int(char num_char);

#endif
