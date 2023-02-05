#include <cstring>
#include <cstdarg>
#include <string>
#include <stdexcept>

#include "formatting.h"


std::string get_formatted_string(std::string& format, ...) {
    va_list args;
    va_start(args, format);
    std::string string = vget_formatted_string(format, args);
    va_end(args);
    return string;
}

std::string vget_formatted_string(std::string& format, va_list args) {
    va_list writing_args, reading_args;
    va_copy(reading_args, args);
    va_copy(writing_args, reading_args);

    size_t alloc_size = vsnprintf(NULL, 0, format.c_str(), reading_args);
    va_end(reading_args);
    char chararr[alloc_size + 1];

    vsnprintf(chararr, alloc_size + 1, format.c_str(), writing_args);
    va_end(writing_args);
    return std::string(chararr);
}

std::string format_time(double time_in_seconds) {
    const int SECONDS_TO_MINUTES = 60;
    const int SECONDS_TO_HOURS = SECONDS_TO_MINUTES * 60;
    const int SECONDS_TO_DAYS = SECONDS_TO_HOURS * 24;

    const long units[4] = { SECONDS_TO_DAYS, SECONDS_TO_HOURS, SECONDS_TO_MINUTES, 1 };
    long conversions[4];
    for (int i = 0; i < 4; i++) {
        conversions[i] = time_in_seconds / units[i];
        time_in_seconds -= conversions[i] * units[i];
    }

    return std::to_string(conversions[1]) + ":" + std::to_string(conversions[2]) + ":" + std::to_string(conversions[3]);
}