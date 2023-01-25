#include <wmath.h>
#include <cstdlib>

double frand() {
    return rand() / (double)RAND_MAX;
}

int signum(int num) {
    if (num < 0) return -1;
    if (num > 0) return 1;
    return 0;
}

int signum(double num) {
    if (num < 0.0) return -1;
    if (num > 0.0) return 1;
    return 0;
}

int clamp(int value, int bounds1, int bounds2) {
    int lower = std::min(bounds1, bounds2);
    int upper = std::max(bounds1, bounds2);
    return std::max(lower, std::min(upper, value));
}

double clamp(double value, double bounds1, double bounds2) {
    double lower = std::min(bounds1, bounds2);
    double upper = std::max(bounds1, bounds2);
    return std::max(lower, std::min(upper, value));
}

long clamp(long value, long bounds1, long bounds2) {
    long lower = std::min(bounds1, bounds2);
    long upper = std::max(bounds1, bounds2);
    return std::max(lower, std::min(upper, value));
}
