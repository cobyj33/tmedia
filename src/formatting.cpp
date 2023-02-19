#include <cstring>
#include <cstdarg>
#include <string>
#include <stdexcept>

#include "formatting.h"
#include "unitconvert.h"


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

std::string format_duration_time_digit(int time_value) { // not in formatting.h
    if (time_value < 10) {
        return "0" + std::to_string(time_value);
    }
    return std::to_string(time_value);
}

std::string format_duration(double time_in_seconds) {
    if (time_in_seconds >= HOURS_TO_SECONDS) {
        return format_time_hh_mm_ss(time_in_seconds);
    }
    return format_time_mm_ss(time_in_seconds);
}


int parse_duration(std::string duration) {
    if (is_hh_mm_ss_duration(duration)) {
        return parse_hh_mm_ss_duration(duration);
    } else if (is_mm_ss_duration(duration)) {
        return parse_mm_ss_duration(duration);
    }
    throw std::runtime_error("Cannot parse duration " + duration + " as duration format could not be found in implemented format types");
}

bool is_duration(std::string duration) {
    return is_hh_mm_ss_duration(duration) || is_mm_ss_duration(duration);  
}

std::string format_time_hh_mm_ss(double time_in_seconds) {
    const int hours = time_in_seconds * SECONDS_TO_HOURS;
    time_in_seconds -= hours * HOURS_TO_SECONDS;
    const int minutes = time_in_seconds * SECONDS_TO_MINUTES;
    time_in_seconds -= minutes * MINUTES_TO_SECONDS;
    const int seconds = (int)(time_in_seconds);

    return format_duration_time_digit(hours) + ":" + format_duration_time_digit(minutes) + ":" + format_duration_time_digit(seconds);
}

int parse_hh_mm_ss_duration(std::string formatted_duration) {
    if (is_hh_mm_ss_duration(formatted_duration)) {
        std::string hours_str = formatted_duration.substr(0, formatted_duration.length() - 6);
        std::string minutes_str = formatted_duration.substr(formatted_duration.length() - 5, 2);
        std::string seconds_str = formatted_duration.substr(formatted_duration.length() - 2, 2);
        return std::stoi(hours_str) * HOURS_TO_SECONDS + std::stoi(minutes_str) * MINUTES_TO_SECONDS + std::stoi(seconds_str);
    }
    throw std::runtime_error("Cannot parse HH:MM:SS duration of " + formatted_duration + ", this duration is not formatted correctly as HH:MM:SS");
}

bool is_hh_mm_ss_duration(std::string formatted_duration) {
    if (formatted_duration.length() >= 8) {
        const int END = formatted_duration.length() - 1;
        const int FIRST_COLON_POSITION = END - 2;
        const int SECOND_COLON_POSITION = END - 5;

        if (formatted_duration[FIRST_COLON_POSITION] == ':' && formatted_duration[SECOND_COLON_POSITION] == ':') {
            
            for (int i = 0; i < formatted_duration.length(); i++) {
                if (!std::isdigit(formatted_duration[i]) && !(i == FIRST_COLON_POSITION || i == SECOND_COLON_POSITION )) {
                    return false;
                }
            }

            return true;
            
        }

    }
    return false;
}

std::string format_time_mm_ss(double time_in_seconds) {
    const int minutes = time_in_seconds * SECONDS_TO_MINUTES;
    time_in_seconds -= minutes * MINUTES_TO_SECONDS;
    const int seconds = (int)(time_in_seconds);
    return format_duration_time_digit(minutes) + ":" + format_duration_time_digit(seconds);
}

int parse_mm_ss_duration(std::string formatted_duration) {
    if (is_mm_ss_duration(formatted_duration)) {
        std::string minutes_str = formatted_duration.substr(0, formatted_duration.length() - 3);
        std::string seconds_str = formatted_duration.substr(formatted_duration.length() - 2, 2);
        return std::stoi(minutes_str) * MINUTES_TO_SECONDS + std::stoi(seconds_str);
    }
    throw std::runtime_error("Cannot parse MM:SS duration of " + formatted_duration + ", this duration is not formatted correctly as MM:SS");
}


bool is_mm_ss_duration(std::string formatted_duration) {
    if (formatted_duration.length() >= 5) {
        const int END = formatted_duration.length() - 1;
        const int COLON_POSITION = END - 2;
        if (formatted_duration[COLON_POSITION] == ':') {

            for (int i = 0; i < formatted_duration.length(); i++) {
                if (!std::isdigit(formatted_duration[i]) && i != COLON_POSITION) {
                    return false;
                }
            }

            return true;
        }
    }
    return false;
}