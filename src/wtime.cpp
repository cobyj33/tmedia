#include <wtime.h>
#include "macros.h"
#include <chrono>

double clock_sec() {
    return (double)(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()).time_since_epoch().count()) * NANOSECONDS_TO_SECONDS;  
}
