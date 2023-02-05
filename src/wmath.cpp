#include <cstdlib>
#include <stdexcept>

#include "wmath.h"

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

bool in_range(int value, int bounds1, int bounds2) {
    return std::min(bounds1, bounds2) <= value && std::max(bounds1, bounds2) >= value;
}

bool in_range(double value, double bounds1, double bounds2) {
    return std::min(bounds1, bounds2) <= value && std::max(bounds1, bounds2) >= value;
}

bool in_range(long value, long bounds1, long bounds2) {
    return std::min(bounds1, bounds2) <= value && std::max(bounds1, bounds2) >= value;
}

int get_next_nearest_perfect_square(double num) {
    double nsqrt = std::sqrt(num);
    if (nsqrt == (int)nsqrt) {
        return std::pow(std::ceil(std::sqrt(num) + 1), 2);
    }

    return std::pow(std::ceil(std::sqrt(num)), 2);
}

int get_next_nearest_perfect_square(int num) {
    double nsqrt = std::sqrt(num);
    if (nsqrt == (int)nsqrt) {
        return std::pow(std::ceil(std::sqrt(num) + 1), 2);
    }

    return std::pow(std::ceil(std::sqrt(num)), 2);
}

int digit_char_to_int(char num_char) {
    if (num_char >= '0' && num_char <= '9') {
        return (int)num_char - 48;
    }
    throw std::runtime_error("Attempted to convert non-digit ascii character to number, " + num_char);
}