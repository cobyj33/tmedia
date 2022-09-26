#include "macros.h"
#include <bits/time.h>
#include <wtime.h>
#include <time.h>

double clock_sec() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (double)tp.tv_sec + (double)tp.tv_nsec / SECONDS_TO_NANOSECONDS;
}
