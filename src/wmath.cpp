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

int fsignum(double num) {
    if (num < 0.0) return -1;
    if (num > 0.0) return 1;
    return 0;
}
