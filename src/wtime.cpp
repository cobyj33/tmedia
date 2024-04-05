#include "wtime.h"

#include "unitconvert.h"

#include <chrono>

double sys_clk_sec() {
  return static_cast<double>(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()).time_since_epoch().count()) * NANOSECONDS_TO_SECONDS;  
}

std::chrono::nanoseconds secs_to_chns(double seconds) {
  return std::chrono::nanoseconds(static_cast<long>(seconds * SECONDS_TO_NANOSECONDS));
}