#include <cstdlib>
#include "audio.h"

uint8_t normalized_float_sample_to_uint8(float num) {
    return (255.0 / 2.0) * (num + 1.0);
}

float uint8_sample_to_normalized_float(uint8_t sample) {
    return ((float)sample - 128.0) / 128.0;
}