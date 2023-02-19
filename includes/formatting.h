#ifndef ASCII_VIDEO_STRING_FORMATTING
#define ASCII_VIDEO_STRING_FORMATTING

#include <string>
#include <cstdarg>

typedef struct DurationData {
    int hours;
    int minutes;
    int seconds;
} DurationData;

std::string get_formatted_string(std::string& format, ...);
std::string vget_formatted_string(std::string& format, va_list args);

std::string format_duration(double time_in_seconds);

std::string format_time_hh_mm_ss(double time_in_seconds);
int parse_hh_mm_ss_duration(std::string formatted_duration);
bool is_hh_mm_ss_duration(std::string formatted_duration);

std::string format_time_mm_ss(double time_in_seconds);
int parse_mm_ss_duration(std::string formatted_duration);
bool is_mm_ss_duration(std::string formatted_duration);

int parse_duration(std::string duration);
bool is_duration(std::string duration);


#endif