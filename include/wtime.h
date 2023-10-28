#ifndef ASCII_VIDEO_WRAPPER_TIME
#define ASCII_VIDEO_WRAPPER_TIME

#include <chrono>

/**
 * @return Return the current system time in seconds
 */
double system_clock_sec();

std::chrono::nanoseconds seconds_to_chrono_nanoseconds(double seconds);

#endif
