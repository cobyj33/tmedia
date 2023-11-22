#ifndef TMEDIA_FORMATTING_H
#define TMEDIA_FORMATTING_H

#include <string>
#include <vector>

typedef struct DurationData {
  int hours;
  int minutes;
  int seconds;
} DurationData;

std::string get_formatted_string(std::string format, ...);
std::string vget_formatted_string(std::string& format, va_list args);

std::string format_duration(double time_in_seconds);

std::string format_time_hh_mm_ss(double time_in_seconds); // ex: 13:03:45
int parse_h_mm_ss_duration(std::string formatted_duration);
bool is_h_mm_ss_duration(std::string formatted_duration);

std::string format_time_mm_ss(double time_in_seconds); // ex: 32:45
int parse_m_ss_duration(std::string formatted_duration);
bool is_m_ss_duration(std::string formatted_duration);

// std::string format_time_hh_mm_ss_SSS(double time_in_seconds); // ex: 00:32:09.026
// int parse_h_mm_ss_SSS_duration(std::string formatted_duration);
// bool is_h_mm_ss_SSS_duration(std::string formatted_duration);

int parse_duration(std::string duration);
bool is_duration(std::string duration);

std::string double_to_fixed_string(double num, int decimal_places);

bool is_int_str(std::string str);

std::string to_filename(const std::string& path);

std::string format_list(std::vector<std::string> items, std::string conjunction);

std::string str_bound(const std::string& str, std::size_t max_size);
std::string vsprintf_str(const char* format, va_list args);
std::string sprintf_str(const char* format, ...);
std::string str_capslock(const std::string& str);

#endif