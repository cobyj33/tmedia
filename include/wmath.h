#ifndef ASCII_VIDEO_WRAPPER_MATH
#define ASCII_VIDEO_WRAPPER_MATH

#include <cmath>
#include <algorithm>

/**
 * @brief Return the sign of the inputted number
 * 
 * @param num 
 * @return -1 if the number is negative, 1 if the number is positive, 0 if the number is 0
 */
int signum(int num);

/**
 * @brief Return the sign of the inputted number
 * 
 * @param num 
 * @return -1 if the number is negative, 1 if the number is positive, 0 if the number is 0
 */
int signum(double num);

/**
 * @brief return a random double between zero and 1
 */
double frand();

/**
 * @brief Clamp the value of a number between two values
 * @note The bounds do not have to be in order from lowest to largest, clamp(2, 4, 5) and clamp (2, 5, 4) will return the same output
 * 
 * If the value is in bounds, clamp will return the value
 * If the value is under the bounds, clamp will return the lowest bounded value
 * If the value is greater than the bounds, clamp will return the highest bounded value
 * 
 * @example clamp(2, 3, 5) == 3
 * @example clamp(7, 3, 5) == 5
 * @example clamp(4, 3, 5) == 4
 * 
 * @param value The value to be clamped
 * @param bounds_1 A bound to clamp the value by
 * @param bounds_2 A bound to clamp the value by
 * @return The number clamped between the two values
 */
int clamp(int value, int bounds_1, int bounds_2);


/**
 * @brief Clamp the value of a number between two values
 * @note The bounds do not have to be in order from lowest to largest, clamp(2, 4, 5) and clamp (2, 5, 4) will return the same output
 * 
 * If the value is in bounds, clamp will return the value
 * If the value is under the bounds, clamp will return the lowest bounded value
 * If the value is greater than the bounds, clamp will return the highest bounded value
 * 
 * @example clamp(2, 3, 5) == 3
 * @example clamp(7, 3, 5) == 5
 * @example clamp(4, 3, 5) == 4
 * 
 * @param value The value to be clamped
 * @param bounds_1 A bound to clamp the value by
 * @param bounds_2 A bound to clamp the value by
 * @return The number clamped between the two values
 */
double clamp(double value, double bounds_1, double bounds_2);


/**
 * @brief Clamp the value of a number between two values
 * @note The bounds do not have to be in order from lowest to largest, clamp(2, 4, 5) and clamp (2, 5, 4) will return the same output
 * 
 * If the value is in bounds, clamp will return the value
 * If the value is under the bounds, clamp will return the lowest bounded value
 * If the value is greater than the bounds, clamp will return the highest bounded value
 * 
 * @example clamp(2, 3, 5) == 3
 * @example clamp(7, 3, 5) == 5
 * @example clamp(4, 3, 5) == 4
 * 
 * @param value The value to be clamped
 * @param bounds_1 A bound to clamp the value by
 * @param bounds_2 A bound to clamp the value by
 * @return The number clamped between the two values
 */
long clamp(long value, long bounds_1, long bounds_2);

bool in_range(int value, int bounds_1, int bounds_2);
bool in_range(double value, double bounds_1, double bounds_2);
bool in_range(long value, long bounds_1, long bounds_2);

int get_next_nearest_perfect_square(double num);
int get_next_nearest_perfect_square(int num);

int digit_char_to_int(char num_char);

#endif
