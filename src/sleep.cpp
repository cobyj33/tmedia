#include "sleep.h"

#include "unitconvert.h"

#include <thread>

void sleep_for_ns(long nanoseconds) {
  std::this_thread::sleep_for(std::chrono::nanoseconds(nanoseconds));
}

void sleep_for_ms(long ms) {
  sleep_for_ns((double)ms * MILLISECONDS_TO_NANOSECONDS);
}

void sleep_for_sec(double secs) {
  sleep_for_ns((long)(secs * SECONDS_TO_NANOSECONDS));
}
