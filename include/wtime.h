#ifndef TMEDIA_W_TIME_H
#define TMEDIA_W_TIME_H

#include "unitconvert.h"

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
inline double sys_clk_sec() {
  return static_cast<double>(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()).time_since_epoch().count()) * NANOSECONDS_TO_SECONDS;  
}

inline std::chrono::nanoseconds secs_to_chns(double seconds) {
  return std::chrono::nanoseconds(static_cast<long>(seconds * SECONDS_TO_NANOSECONDS));
}

#endif
