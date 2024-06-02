#include <tmedia/util/thread.h>

#include <tmedia/util/unitconvert.h>

#include <thread>

#if defined(__linux__) // needed for setting current thread's name
#include <sys/prctl.h>
#endif


// Since there is no C++ 11 supported cross-platform way
// to give a current thread a name viewable by other
// programs on the system, we have to handle each platform
// in a specific way
void name_current_thread(std::string_view name) {
  #if defined(__linux__)
  // While we could use pthreads' pthread_setname_np function
  // to set the current thread name on a thread's pthread_t
  // handle, these functions are actually nonstandard GNU
  // extensions, and aren't guaranteed to be a part of
  // pthread.h anyway. Additionally, it would require a
  // direct linking to pthread in the build system to find
  // and link to libpthread, which isn't necessary when we can just call
  // the system directly and libpthread may not even have the extension
  // anyway.
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
