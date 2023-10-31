#ifndef TMEDIA_TMEDIA_STRING_H
#define TMEDIA_TMEDIA_STRING_H

#include <cstdarg>
#include <string>
#include <cstddef>

std::string str_bound(const std::string& str, std::size_t max_size);
std::string vsprintf_str(const char* format, va_list args);
std::string sprintf_str(const char* format, ...);
std::string str_capslock(const std::string& str);

#endif