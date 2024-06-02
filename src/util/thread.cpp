#include <tmedia/util/thread.h>

#include <tmedia/util/unitconvert.h>

#include <thread>

#if defined(__linux__) // needed for setting current thread's name
#include <sys/prctl.h>
#endif


void name_current_thread(std::string_view name) {
  #if defined(__linux__)
  prctl(PR_SET_NAME, name.data(), 0, 0, 0);
  #endif
}

void sleep_for_ns(long ns) {
  std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
}

void sleep_for_ms(long ms) {
  sleep_for_ns(ms * MILLISECONDS_TO_NANOSECONDS);
}

void sleep_for_sec(double secs) {
  sleep_for_ns(static_cast<long>(secs * SECONDS_TO_NANOSECONDS));
}
