#ifndef TMEDIA_W_TIME_H
#define TMEDIA_W_TIME_H

#include <chrono>

/**
 * @return Return the current system time in seconds
 */
double system_clock_sec();

std::chrono::nanoseconds seconds_to_chrono_nanoseconds(double seconds);

#endif
