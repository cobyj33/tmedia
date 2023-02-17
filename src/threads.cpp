#include <thread>

#include "threads.h"
#include "unitconvert.h"

void sleep_for(long nanoseconds) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(nanoseconds));
}

void sleep_quick() {
    std::this_thread::sleep_for(std::chrono::nanoseconds(1));
}

void sleep_for_ms(long ms) {
    sleep_for((double)ms * MILLISECONDS_TO_NANOSECONDS);
}

void sleep_for_sec(long secs) {
    sleep_for(secs * SECONDS_TO_NANOSECONDS);
}

void sleep_for_sec(double secs) {
    sleep_for((long)(secs * SECONDS_TO_NANOSECONDS));
}
