#include "wtime.h"

#include "unitconvert.h"

#include <chrono>

double system_clock_sec() {
  return (double)(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()).time_since_epoch().count()) * NANOSECONDS_TO_SECONDS;  
}

std::chrono::nanoseconds seconds_to_chrono_nanoseconds(double seconds) {
  return std::chrono::nanoseconds((long)(seconds * SECONDS_TO_NANOSECONDS));
}