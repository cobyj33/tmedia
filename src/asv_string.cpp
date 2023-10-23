#include "asv_string.h"

#include <cstdarg>
#include <stdexcept>
#include <string>
#include <cstdlib>

std::string snprintf_str(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string str = vsnprintf_str(format, args);
    va_end(args);
    return str;
}

std::string vsnprintf_str(const char* format, va_list args) {
    va_list writing_args, reading_args;
    va_copy(reading_args, args);
    va_copy(writing_args, args);

    int alloc_size = vsnprintf(nullptr, 0, format, reading_args);
    va_end(reading_args);
    if (alloc_size < 0) {
      va_end(writing_args);
      throw std::runtime_error("[vsnprintf_str] vsnprintf error return value: " + std::to_string(alloc_size));
    }

    char* cstr = (char*)malloc(sizeof(char) * (alloc_size + 1));
    if (cstr == nullptr) {
        va_end(writing_args);
        throw std::bad_alloc();
    }

    vsnprintf(cstr, alloc_size + 1, format, writing_args);
    va_end(writing_args);

    std::string string(cstr);
    free(cstr);
    return string;
}

std::string str_bound(const std::string& str, std::size_t max_size) {
  if (str.length() > max_size) {
    return str.substr(0, max_size);
  }
  return str;
}