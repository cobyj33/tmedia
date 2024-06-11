#ifndef TMEDIA_FORMATTING_H
#define TMEDIA_FORMATTING_H

#include <string>
#include <string_view>
#include <vector>

/**
 * tmedia general string formatting and manipulation routines.
*/

/**
 * vsprintf adopted to fill a std::string buffer.
 * See C standard vsprintf documentation for more details!
*/
std::string vsprintf_str(const char* format, va_list args);

/**
 * sprintf adopted to fill a std::string buffer.
 * See C standard sprintf documentation for more details!
*/
std::string sprintf_str(const char* format, ...);

/**
 * formats a duration in seconds into a duration string depending on the time
 * in seconds given.
 * If the amount of seconds of time_in_seconds is greater than or equal to 3600
 * (the amount of seconds in an hour), the string will be in HH:MM:SS format.
 * Otherwise, the string will be in MM:SS format
*/
std::string format_duration(double time_in_seconds);

/**
 * Formats a duration in the HH:MM:SS duration format.
 * If there is
*/
std::string format_time_hh_mm_ss(double time_in_seconds); // ex: 13:03:45

/**
 * Strictly parses a H:MM:SS formatted string into the number of seconds
 * contained within.
 *
 * Throws an error if the duration is not formatted properly in H:MM:SS format.
 * This function does not expect any padding or white space in the string at
 * all.
 *
 * is_h_mm_ss_duration should be called beforehand, especially with user input,
 * in order to see if the string is in the correct format.
*/
int parse_h_mm_ss_duration(std::string_view formatted_duration);
bool is_h_mm_ss_duration(std::string_view formatted_duration);

std::string format_time_mm_ss(double time_in_seconds); // ex: 32:45
int parse_m_ss_duration(std::string_view formatted_duration);
bool is_m_ss_duration(std::string_view formatted_duration);

bool strisi32(std::string_view) noexcept;
int strtoi32(std::string_view);

bool strisdouble(std::string_view) noexcept;
double strtodouble(std::string_view);

// std::string format_time_hh_mm_ss_SSS(double time_in_seconds); // ex: 00:32:09.026
// int parse_h_mm_ss_SSS_duration(std::string formatted_duration);
// bool is_h_mm_ss_SSS_duration(std::string formatted_duration);

int parse_duration(std::string_view duration);
bool is_duration(std::string_view duration);

std::string double_to_fixed_string(double num, int decimal_places);

bool is_int_str(std::string_view str);

std::string format_list(const std::vector<std::string>& items, std::string_view conjunction);

double parse_percentage(std::string_view percentage);
bool is_percentage(std::string_view percentage);

std::string strv_bound(std::string_view str, std::size_t max_size);
std::string str_bound(std::string_view str, std::size_t max_size);
std::string str_capslock(std::string_view str);
std::string_view strv_trim(std::string_view src, std::string_view trimchars);
std::string str_trim(std::string_view src, std::string_view trimchars);

/**
 * Note that the string_views returned will be substringed from the original
 * string_view. This means they will NOT be NULL-terminated and you should
 * NOT use them as c-strings when passing the data around. However, if
 * strvsplit is used with other functions or methods that expect
 * std::string_view objects, it should be fine
*/
std::vector<std::string_view> strvsplit(std::string_view s, char delim);

std::vector<std::string> strsplit(std::string_view s, char delim);

/**
 * Read a string_view until a specific character is hit, and return the
 * substring from the start of the string to the hit character (exclusive).
*/
std::string_view str_until(std::string_view s, char ch);

#endif
