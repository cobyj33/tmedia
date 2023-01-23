#include <ctime>
#include <wtime.h>
#include "macros.h"

extern "C" {
#include <bits/time.h>
}

double clock_sec() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (double)tp.tv_sec + (double)tp.tv_nsec / SECONDS_TO_NANOSECONDS;
}
