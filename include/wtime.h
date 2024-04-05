#ifndef TMEDIA_W_TIME_H
#define TMEDIA_W_TIME_H

#include <chrono>

/**
 * Wrappers for boilerplate of fetching the current time
 * from the system monotonic clock and converting between
 * std::chrono units
*/

/**
 * Return the current system time in seconds from the system's
 * monotonic clock
 */
double sys_clk_sec();

std::chrono::nanoseconds secs_to_chns(double seconds);

#endif
