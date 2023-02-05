#ifndef ASCII_VIDEO_STRING_FORMATTING
#define ASCII_VIDEO_STRING_FORMATTING

#include <string>
#include <cstdarg>
std::string get_formatted_string(std::string& format, ...);
std::string vget_formatted_string(std::string& format, va_list args);
std::string format_time(double time_in_seconds);

#endif