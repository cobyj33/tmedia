#include <tmedia/util/sleep.h>

#include <tmedia/util/unitconvert.h>

#include <thread>

void sleep_for_ns(long ns) {
  std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
}

void sleep_for_ms(long ms) {
  sleep_for_ns(ms * MILLISECONDS_TO_NANOSECONDS);
}

void sleep_for_sec(double secs) {
  sleep_for_ns(static_cast<long>(secs * SECONDS_TO_NANOSECONDS));
}
