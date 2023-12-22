#ifndef TMEDIA_FORMATTING_H
#define TMEDIA_FORMATTING_H

#include <string>
#include <string_view>
#include <vector>

typedef struct DurationData {
  int hours;
  int minutes;
  int seconds;
} DurationData;

std::string vsprintf_str(const char* format, va_list args);
std::string sprintf_str(const char* format, ...);

std::string format_duration(double time_in_seconds);

std::string format_time_hh_mm_ss(double time_in_seconds); // ex: 13:03:45
int parse_h_mm_ss_duration(std::string formatted_duration);
bool is_h_mm_ss_duration(std::string formatted_duration);

std::string format_time_mm_ss(double time_in_seconds); // ex: 32:45
int parse_m_ss_duration(std::string formatted_duration);
bool is_m_ss_duration(std::string formatted_duration);

bool strisi32(std::string_view) noexcept;
int strtoi32(std::string_view);

bool strisdouble(std::string_view) noexcept;
double strtodouble(std::string_view);

// std::string format_time_hh_mm_ss_SSS(double time_in_seconds); // ex: 00:32:09.026
// int parse_h_mm_ss_SSS_duration(std::string formatted_duration);
// bool is_h_mm_ss_SSS_duration(std::string formatted_duration);

int parse_duration(std::string duration);
bool is_duration(std::string duration);

std::string double_to_fixed_string(double num, int decimal_places);

bool is_int_str(std::string_view str);

std::string to_filename(std::string_view path);
std::string to_file_ext(std::string_view path);

std::string format_list(std::vector<std::string> items, std::string_view conjunction);

double parse_percentage(std::string_view percentage);
bool is_percentage(std::string_view percentage);

std::string str_bound(std::string_view str, std::size_t max_size);
std::string str_capslock(std::string_view str);
std::string str_trim(std::string_view src, std::string_view trimchars);
std::vector<std::string_view> strvsplit(std::string_view s, char delim);
std::vector<std::string> strsplit(std::string_view s, char delim);

std::string_view str_until(std::string_view s, char ch);

#endif