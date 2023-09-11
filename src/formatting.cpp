#include <cstring>
#include <cstdarg>
#include <string>
#include <stdexcept>
#include <cmath>

#include <iomanip>
#include <sstream>

#include "formatting.h" 
#include "unitconvert.h"

std::string double_to_fixed_string(double num, int decimal_places) {
  if (decimal_places < 0) {
    throw std::runtime_error("Cannot produce double string with negative decimal places");
  } else if (decimal_places == 0) {
    return std::to_string((long)num);
  }

  std::stringstream stream;
  stream << std::fixed << std::setprecision(decimal_places) << num;
  std::string s = stream.str();
  return s;
}


std::string get_formatted_string(std::string format, ...) {
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
  char* chararr = new char[alloc_size + 1];

  vsnprintf(chararr, alloc_size + 1, format.c_str(), writing_args);
  va_end(writing_args);
  std::string output(chararr);
  delete[] chararr;
  return output;
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
  if (is_h_mm_ss_duration(duration)) {
    return parse_h_mm_ss_duration(duration);
  } else if (is_m_ss_duration(duration)) {
    return parse_m_ss_duration(duration);
  }
  throw std::runtime_error("Cannot parse duration " + duration + " as duration format could not be found in implemented format types");
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
  if (is_h_mm_ss_duration(formatted_duration)) {
    const int END = formatted_duration.length() - 1;
    const int MM_SS_COLON_POSITION = END - 2;
    const int HH_MM_COLON_POSITION = END - 5;
    std::string hours_str = formatted_duration.substr(0, HH_MM_COLON_POSITION);
    std::string minutes_str = formatted_duration.substr(HH_MM_COLON_POSITION + 1, 2);
    std::string seconds_str = formatted_duration.substr(MM_SS_COLON_POSITION + 1, 2);
    return std::stoi(hours_str) * HOURS_TO_SECONDS + std::stoi(minutes_str) * MINUTES_TO_SECONDS + std::stoi(seconds_str);
  }
  throw std::runtime_error("Cannot parse H:MM:SS duration of " + formatted_duration + ", this duration is not formatted correctly as H:MM:SS");
}

bool is_h_mm_ss_duration(std::string formatted_duration) {
  if (formatted_duration.length() >= 7) {
    const int END = formatted_duration.length() - 1;
    const int MM_SS_COLON_POSITION = END - 2;
    const int HH_MM_COLON_POSITION = END - 5;

    if (formatted_duration[MM_SS_COLON_POSITION] == ':' && formatted_duration[HH_MM_COLON_POSITION] == ':') {
      for (int i = 0; i < (int)formatted_duration.length(); i++) {
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
  }

  return false;
}

std::string format_time_mm_ss(double time_in_seconds) {
  const int minutes = time_in_seconds * SECONDS_TO_MINUTES;
  time_in_seconds -= minutes * MINUTES_TO_SECONDS;
  const int seconds = (int)(time_in_seconds);
  return format_duration_time_digit(minutes) + ":" + format_duration_time_digit(seconds);
}

int parse_m_ss_duration(std::string formatted_duration) {
  if (is_m_ss_duration(formatted_duration)) {
    const int END = formatted_duration.length() - 1;
    const int COLON_POSITION = END - 2;
    std::string minutes_str = formatted_duration.substr(0, COLON_POSITION);
    std::string seconds_str = formatted_duration.substr(COLON_POSITION + 1, 2);
    return std::stoi(minutes_str) * MINUTES_TO_SECONDS + std::stoi(seconds_str);
  }
  throw std::runtime_error("Cannot parse MM:SS duration of " + formatted_duration + ", this duration is not formatted correctly as MM:SS");
}


bool is_m_ss_duration(std::string formatted_duration) {
  if (formatted_duration.length() >= 4) {
    const int END = formatted_duration.length() - 1;
    const int COLON_POSITION = END - 2;

    if (formatted_duration[COLON_POSITION] == ':') {
      for (int i = 0; i < (int)formatted_duration.length(); i++) {
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
  }
  return false;
}

bool is_int_str(std::string str) {
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