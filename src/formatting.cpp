#include "formatting.h"
 
#include "unitconvert.h"
#include "funcmac.h"

#include <fmt/format.h>

#include <cstdarg>
#include <cstddef>
#include <vector>
#include <string>
#include <string_view>
#include <stdexcept>
#include <iomanip> // std::setprecision
#include <sstream>
#include <filesystem>
#include <cctype>
#include <climits>
#include <cfloat>
#include <cstdlib>

/**
 * With the fmt library, this might all be irrelevant now. We'll see
*/

std::string double_to_fixed_string(double num, int decimal_places) {
  if (decimal_places < 0) {
    throw std::runtime_error(fmt::format("[{}] Cannot produce double string "
    "with negative decimal places", FUNCDINFO));
  } else if (decimal_places == 0) {
    return std::to_string((long)num);
  }

  std::stringstream stream;
  stream << std::fixed << std::setprecision(decimal_places) << num;
  std::string s = stream.str();
  return s;
}

std::string_view strv_trim(std::string_view src, std::string_view trimchars) {
  if (src.empty()) return ""; // needed to guard against src.length() - 1 flowing from 0 to SIZE_MAX
  if (trimchars.empty()) return std::string(src);

  std::size_t start_index = 0;
  std::size_t end_index = src.length() - 1;

  while (start_index < src.length() - 1) {
    if (trimchars.find(src[start_index]) == std::string::npos) break;
    start_index++;
  }

  while (end_index > 0) {
    if (trimchars.find(src[end_index]) == std::string::npos) break;
    end_index--;
  }

  if (end_index < start_index) return "";
  return src.substr(start_index, end_index - start_index + 1);
}

std::string str_trim(std::string_view src, std::string_view trimchars) {
  return std::string(strv_trim(src, trimchars));
}

std::string sprintf_str(const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::string string = vsprintf_str(format, args);
  va_end(args);
  return string;
}

std::string vsprintf_str(const char* format, va_list args) {
  va_list writing_args, reading_args;
  va_copy(reading_args, args);
  va_copy(writing_args, reading_args);

  int alloc_size = vsnprintf(NULL, 0, format, reading_args);
  va_end(reading_args);
  if (alloc_size < 0) {
    va_end(writing_args);
    throw std::runtime_error(fmt::format("[{}] vsnprintf error: {}.",
    FUNCDINFO, alloc_size));
  }

  char* chararr = new char[alloc_size + 1];

  int success = vsnprintf(chararr, alloc_size + 1, format, writing_args);
  va_end(writing_args);
  if (success < 0) {
    delete[] chararr;
    throw std::runtime_error(fmt::format("[{}] vsnprintf error: {}.",
    FUNCDINFO, alloc_size));
  }

  std::string output(chararr);
  delete[] chararr;
  return output;
}

std::string format_duration_time_digit(int time_value) { // not in formatting.h
  return (time_value < 10 ? "0" : "") + std::to_string(time_value);
}

std::string format_duration(double time_in_seconds) {
  if (time_in_seconds >= HOURS_TO_SECONDS) {
    return format_time_hh_mm_ss(time_in_seconds);
  }
  return format_time_mm_ss(time_in_seconds);
}


int parse_duration(std::string duration) {
  if (is_h_mm_ss_duration(duration)) {
    return parse_h_mm_ss_duration(duration);
  } else if (is_m_ss_duration(duration)) {
    return parse_m_ss_duration(duration);
  }
  throw std::runtime_error(fmt::format("[{}] Cannot parse duration {} as "
  "duration format could not be found in implemented format types",
  FUNCDINFO, duration));
}

bool is_duration(std::string duration) {
  return is_h_mm_ss_duration(duration) || is_m_ss_duration(duration);  
}

std::string format_time_hh_mm_ss(double time_in_seconds) {
  const int hours = time_in_seconds * SECONDS_TO_HOURS;
  time_in_seconds -= hours * HOURS_TO_SECONDS;
  const int minutes = time_in_seconds * SECONDS_TO_MINUTES;
  time_in_seconds -= minutes * MINUTES_TO_SECONDS;
  const int seconds = (int)(time_in_seconds);

  return format_duration_time_digit(hours) + ":" + format_duration_time_digit(minutes) + ":" + format_duration_time_digit(seconds);
}

int parse_h_mm_ss_duration(std::string formatted_duration) {
  if (!is_h_mm_ss_duration(formatted_duration)) {
    throw std::runtime_error(fmt::format("[{}] Cannot parse H:MM:SS duration "
    "of {}, this duration is not formatted correctly as H:MM:SS",
    FUNCDINFO, formatted_duration));
  }

  const int END = formatted_duration.length() - 1;
  const int MM_SS_COLON_POSITION = END - 2;
  const int HH_MM_COLON_POSITION = END - 5;
  std::string hours_str = formatted_duration.substr(0, HH_MM_COLON_POSITION);
  std::string minutes_str = formatted_duration.substr(HH_MM_COLON_POSITION + 1, 2);
  std::string seconds_str = formatted_duration.substr(MM_SS_COLON_POSITION + 1, 2);
  return std::stoi(hours_str) * HOURS_TO_SECONDS + std::stoi(minutes_str) * MINUTES_TO_SECONDS + std::stoi(seconds_str);
}

bool is_h_mm_ss_duration(std::string formatted_duration) {
  if (formatted_duration.length() < 7) {
    return false;
  }

  const std::size_t END = formatted_duration.length() - 1;
  const std::size_t MM_SS_COLON_POSITION = END - 2;
  const std::size_t HH_MM_COLON_POSITION = END - 5;

  if (formatted_duration[MM_SS_COLON_POSITION] != ':' || formatted_duration[HH_MM_COLON_POSITION] != ':') {
    return false;
  }

  for (std::size_t i = 0; i < formatted_duration.length(); i++) {
    if (!std::isdigit(formatted_duration[i]) && !(i == MM_SS_COLON_POSITION || i == HH_MM_COLON_POSITION )) {
      return false;
    }
  }

  std::string hours_section = formatted_duration.substr(0, HH_MM_COLON_POSITION);
  std::string minutes_section = formatted_duration.substr(HH_MM_COLON_POSITION + 1, 2);
  std::string seconds_section = formatted_duration.substr(MM_SS_COLON_POSITION + 1, 2);

  if (hours_section.length() > 2) {
    if (hours_section[0] == '0') {
      return false;
    }
  }

  int seconds = std::stoi(seconds_section);
  int minutes = std::stoi(minutes_section);
  
  return seconds < 60 && minutes < 60;
}

std::string format_time_mm_ss(double time_in_seconds) {
  const int minutes = time_in_seconds * SECONDS_TO_MINUTES;
  time_in_seconds -= minutes * MINUTES_TO_SECONDS;
  const int seconds = (int)(time_in_seconds);
  return format_duration_time_digit(minutes) + ":" + format_duration_time_digit(seconds);
}

int parse_m_ss_duration(std::string formatted_duration) {
  if (!is_m_ss_duration(formatted_duration)) {
    throw std::runtime_error(fmt::format("[{}] Cannot parse M:SS duration "
    "of {}, this duration is not formatted correctly as M:SS",
    FUNCDINFO, formatted_duration));
  }

  const int END = formatted_duration.length() - 1;
  const int COLON_POSITION = END - 2;
  std::string minutes_str = formatted_duration.substr(0, COLON_POSITION);
  std::string seconds_str = formatted_duration.substr(COLON_POSITION + 1, 2);
  return std::stoi(minutes_str) * MINUTES_TO_SECONDS + std::stoi(seconds_str);
}


bool is_m_ss_duration(std::string formatted_duration) {
  if (formatted_duration.length() < 4) {
    return false;
  }

  const std::size_t END = formatted_duration.length() - 1;
  const std::size_t COLON_POSITION = END - 2;
  if (formatted_duration[COLON_POSITION] != ':') {
    return false;
  }

  for (std::size_t i = 0; i < formatted_duration.length(); i++) {
    if (!std::isdigit(formatted_duration[i]) && i != COLON_POSITION) {
      return false;
    }
  }

  std::string minutes_section = formatted_duration.substr(0, COLON_POSITION);
  if (minutes_section.length() > 2) {
    if (minutes_section[0] == '0') {
      return false;
    }
  }


  std::string seconds_section = formatted_duration.substr(COLON_POSITION + 1, 2);
  int seconds = std::stoi(seconds_section);
  
  return seconds < 60;
}

bool is_int_str(std::string_view str) {
  if (str.length() == 0) {
    return false;
  }

  for (std::size_t i = 0; i < str.length(); i++) {
    if (str[i] == '-') {
      if (i == 0) {
        if (str.length() == 1) { // the string is "-"
          return false;
        }
        continue;
      }
      return false; // The string has a '-' somewhere besides the start
    }
    
    if (!std::isdigit(str[i])) { // the string has a character that is not a digit or a '-'
      return false;
    }
  }
  
  return true;
}

std::string format_list(std::vector<std::string> items, std::string_view conjunction) {
  if (items.size() == 0) {
    return "";
  }

  if (items.size() == 1) {
    return items[0];
  }

  if (items.size() == 2) {
    std::stringstream sstream;
    sstream << items[0] << " " << conjunction << " " << items[1];
    return sstream.str();
  }

  std::stringstream sstream;
  for (std::size_t i = 0; i < items.size() - 1; i++) {
    sstream << items[i] << ", ";
  }
  sstream << conjunction << " " << items[items.size() - 1];
  return sstream.str();
}



std::string str_bound(std::string_view str, std::size_t max_size) {
  if (str.length() > max_size) {
    return std::string(str.substr(0, max_size));
  }
  return std::string(str);
}

std::string str_capslock(std::string_view str) {
  std::string out;
  for (std::size_t i = 0; i < str.length(); i++) {
    out += std::toupper(str[i]);
  }
  return out;
}

std::vector<std::string_view> strvsplit(std::string_view s, char delim) {
  std::vector<std::string_view> result;
  std::size_t ssstart = 0;

  for (std::size_t ssend = 0; ssend < s.length(); ssend++) {
    if (s[ssend] == delim) {
      if (ssend > ssstart)
      ssstart = ssend;
    }
  }

  if (ssstart < s.length() - 1) 
    result.push_back(s.substr(ssstart, s.length()));

  return result;
}

std::vector<std::string> strsplit(std::string_view s, char delim) {
  std::vector<std::string> result;
  std::size_t ssstart = 0;

  for (std::size_t ssend = 0; ssend < s.length(); ssend++) {
    if (s[ssend] == delim) {
      if (ssend > ssstart)
        result.push_back(std::string(s.substr(ssstart, ssend - ssstart)));
      ssstart = ssend + 1;
    }
  }

  if (s.length() > 0 && ssstart < s.length() - 1) {
    result.push_back(std::string(s.substr(ssstart, s.length() - ssstart)));
  }

  return result;
}

std::string_view str_until(std::string_view s, char ch) {
  std::size_t end = s.length();

  for (std::size_t i = 0; i < s.length(); i++) {
    if (s[i] == ch) {
      end = i;
      break;
    }
  }

  return s.substr(0, end);
}

bool strisi32(std::string_view str) noexcept {
  try {
    (void)strtoi32(str);
    return true;
  } catch (const std::runtime_error& err) {
    return false;
  }
}

int strtoi32(std::string_view str) {
  double out = 0;
  if (str.empty())
    throw std::runtime_error(fmt::format("[{}] Attempted to parse empty "
    "string as i32", FUNCDINFO));
  if (str == "-" || str == "+")
    throw std::runtime_error(fmt::format("[{}] Attempted to parse string with "
    "only a sign as i32: {}.", FUNCDINFO, str));

  int sign = str[0] == '-' ? -1 : 1;

  for (std::size_t i = str[0] == '-' || str[0] == '+' ? 1 : 0; i < str.length(); i++) {
    if (!std::isdigit(str[i]))
      throw std::runtime_error(fmt::format("[{}] Attempted to parse string "
      "with invalid non-digit character: ", FUNCDINFO, str));
    if (out >= (INT_MAX - 9) / 10)
      throw std::runtime_error(fmt::format("[{}] i32 integer overflow: {}, "
      "Must be in the range [ {} -> {} ]", FUNCDINFO, str, INT_MIN, INT_MAX));

    out *= 10;
    out += str[i] - '0';
  }

  return out * sign;
}

bool strisdouble(std::string_view str) noexcept {
  try {
    (void)strtodouble(str);
    return true;
  } catch (const std::runtime_error& err) {
    return false;
  }
}

double strtodouble(std::string_view str) {
  double out = 0.0;

  if (str.empty())
    throw std::runtime_error(fmt::format("[{}] Attempted to parse empty string "
    "as double", FUNCDINFO));
  if (str == "-." || str == "+." || str == "+" || str == "-")
    throw std::runtime_error(fmt::format("[{}] Attempted to parse invalid "
    "signed string: {}.", FUNCDINFO, str));

  int foundDecimal = 0;
  double decimalMultiplier = 0.1;
  double sign = str[0] == '-' ? -1.0 : 1.0;

  for (std::size_t i = str[0] == '-' || str[0] == '+' ? 1 : 0; i < str.length(); i++) {
    if (str[i] == '.') {
      if (foundDecimal)
        throw std::runtime_error(fmt::format("[{}] Attempted to parse string "
        "with multiple decimal points: {}.", FUNCDINFO, str));
      foundDecimal = 1;
    } else if (std::isdigit(str[i])) {
      if (out >= (DBL_MAX - 9) / 10)
        throw std::runtime_error(fmt::format("[{}] double overflow: {}.",
        FUNCDINFO, str));
      
      if (foundDecimal) {
        out = out + (str[i] - '0') * decimalMultiplier;
        decimalMultiplier /= 10.0;
      }
      else {
        out = out * 10.0 + (str[i] - '0'); 
      }

    } else {
      throw std::runtime_error(fmt::format("[{}] Attempted to parse string "
      "with invalid non-digit character: {}.", FUNCDINFO, str));
    }
  }

  return out * sign;
}

bool is_percentage(std::string_view str) {
  try {
    (void)parse_percentage(str);
    return true;
  } catch (const std::runtime_error& err) {
    return false;
  }
}

/**
 * either a float or a float ending with %
*/
double parse_percentage(std::string_view str) {
  if (str.length() > 0 && str[str.length() - 1] == '%') {
    return strtodouble(str.substr(0, str.length() - 1)) / 100.0;
  }
  return strtodouble(str);
}
