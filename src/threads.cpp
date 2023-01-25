#include <threads.h>
#include <unitconvert.h>
#include <thread>


void sleep_for(long nanoseconds) {
    // struct timespec sleep_time = { nanoseconds / (long)SECONDS_TO_NANOSECONDS, nanoseconds % SECONDS_TO_NANOSECONDS  };
    std::this_thread::sleep_for(std::chrono::nanoseconds(nanoseconds));
    // nanosleep(&sleep_time, NULL);
}

void sleep_for_ms(long ms) {
    sleep_for((double)ms * MILLISECONDS_TO_NANOSECONDS);
}

void sleep_for_sec(long secs) {
    sleep_for(secs * SECONDS_TO_NANOSECONDS);
}

void fsleep_for_sec(double secs) {
    sleep_for((long)(secs * SECONDS_TO_NANOSECONDS));
}
