#include <tmedia/util/formatting.h>

#include <catch2/catch_test_macros.hpp>

#include <vector>

constexpr int SECOND = 1;
constexpr int HALF_MINUTE = 30;
constexpr int ONE_MINUTE = 60;
constexpr int HALF_HOUR = 1800;
constexpr int ONE_HOUR = 3600;

template <typename T>
bool are_vectors_equal(std::vector<T> first, std::vector<T> second) {
  if (first.size() != second.size()) return false;

  for (std::size_t i = 0; i < first.size(); i++) {
    if (first[i] != second[i]) return false;
  }

  return true;
}


TEST_CASE("Formatting", "[functions]") {
  SECTION("Is integer string") {

    SECTION("Valid") {
      REQUIRE(is_int_str("12"));
      REQUIRE(is_int_str("-12"));
      REQUIRE(is_int_str("94732"));
      REQUIRE(is_int_str("-232"));
      REQUIRE(is_int_str("243"));
      REQUIRE(is_int_str("-32"));
    }

    SECTION("Invalid") {
      REQUIRE_FALSE(is_int_str("1.2"));
      REQUIRE_FALSE(is_int_str("1-2"));
      REQUIRE_FALSE(is_int_str("12-"));
      REQUIRE_FALSE(is_int_str("0x12"));
      REQUIRE_FALSE(is_int_str("--13"));
      REQUIRE_FALSE(is_int_str(" 345"));
      REQUIRE_FALSE(is_int_str("345 "));
    }
  }
  

  SECTION("H:MM:SS") {
    SECTION("validation") {
      REQUIRE(is_h_mm_ss_duration("00:00:00"));
      REQUIRE(is_h_mm_ss_duration("0:00:00"));
      REQUIRE(is_h_mm_ss_duration("00:00:10"));
      REQUIRE(is_h_mm_ss_duration("00:01:10"));
      REQUIRE(is_h_mm_ss_duration("00:10:10"));
      REQUIRE(is_h_mm_ss_duration("00:01:00"));
      REQUIRE(is_h_mm_ss_duration("00:01:30"));
      REQUIRE(is_h_mm_ss_duration("01:00:10"));
      REQUIRE(is_h_mm_ss_duration("01:01:10"));
      REQUIRE(is_h_mm_ss_duration("70:00:10"));
    }

    SECTION("Incorrect") {
      REQUIRE_FALSE(is_h_mm_ss_duration("00:01"));
      REQUIRE_FALSE(is_h_mm_ss_duration(":00:00:01"));
      REQUIRE_FALSE(is_h_mm_ss_duration("00:00:01:"));
      REQUIRE_FALSE(is_h_mm_ss_duration("00:0.1"));
      REQUIRE_FALSE(is_h_mm_ss_duration("00:01."));
      REQUIRE_FALSE(is_h_mm_ss_duration("0.0:01"));
      REQUIRE_FALSE(is_h_mm_ss_duration("0:0:0")); 
      REQUIRE_FALSE(is_h_mm_ss_duration("1:1:1"));

      REQUIRE_FALSE(is_h_mm_ss_duration("10:00:61")); // seconds greater than 60
      REQUIRE_FALSE(is_h_mm_ss_duration("10:61:00")); // minutes greater than 60
      REQUIRE_FALSE(is_h_mm_ss_duration("10:61:61")); // minutes and seconds greater than 60

      REQUIRE_FALSE(is_h_mm_ss_duration("10:-34:15")); // negative minutes
      REQUIRE_FALSE(is_h_mm_ss_duration("10:34:-15")); // negative seconds
      REQUIRE_FALSE(is_h_mm_ss_duration("-10:34:15")); // negative hours
      REQUIRE_FALSE(is_h_mm_ss_duration("10 34 15")); // whitespace instead of colons
      REQUIRE_FALSE(is_h_mm_ss_duration("-10 34 15")); // negative hours and whitespace instead of colons
    }

    std::string ten_sec = format_time_hh_mm_ss(SECOND * 10);
    REQUIRE(ten_sec == "00:00:10");

    std::string ten_half_sec = format_time_hh_mm_ss(SECOND * 10.5);
    REQUIRE(ten_half_sec == "00:00:10");

    std::string min_format = format_time_hh_mm_ss(ONE_MINUTE);
    REQUIRE(min_format == "00:01:00");

    std::string min_half_format = format_time_hh_mm_ss(ONE_MINUTE + HALF_MINUTE);
    REQUIRE(min_half_format == "00:01:30");

    std::string hour_format = format_time_hh_mm_ss(ONE_HOUR);
    REQUIRE(hour_format == "01:00:00");

    std::string hour_min_30_format = format_time_hh_mm_ss(ONE_HOUR + ONE_MINUTE + HALF_MINUTE);
    REQUIRE(hour_min_30_format == "01:01:30");

    std::string seventy_hours_format = format_time_hh_mm_ss(ONE_HOUR * 70);
    REQUIRE(seventy_hours_format == "70:00:00");
  }

  SECTION("mm:ss") {

    SECTION("validation") {
      REQUIRE(is_m_ss_duration("00:00"));
      REQUIRE(is_m_ss_duration("00:10"));
      REQUIRE(is_m_ss_duration("01:10"));
      REQUIRE(is_m_ss_duration("10:10"));
      REQUIRE(is_m_ss_duration("01:00"));
      REQUIRE(is_m_ss_duration("01:30"));
      REQUIRE(is_m_ss_duration("00:10"));
      REQUIRE(is_m_ss_duration("01:10"));
      REQUIRE(is_m_ss_duration("61:10"));
      REQUIRE(is_m_ss_duration("0:10"));
      REQUIRE(is_m_ss_duration("1:10"));
    }

    SECTION("Invalid") {
      REQUIRE_FALSE(is_m_ss_duration("00:61")); // seconds >= 60
      REQUIRE_FALSE(is_m_ss_duration("00:60")); // seconds >= 60
      REQUIRE_FALSE(is_m_ss_duration("00:99"));
      REQUIRE_FALSE(is_m_ss_duration("00:10:"));
      REQUIRE_FALSE(is_m_ss_duration(":00:10"));
      REQUIRE_FALSE(is_m_ss_duration("000:10"));
      REQUIRE_FALSE(is_m_ss_duration("00:00:10"));
      
      REQUIRE_FALSE(is_m_ss_duration("-00:10"));
      REQUIRE_FALSE(is_m_ss_duration("-000:10"));
      REQUIRE_FALSE(is_m_ss_duration("-00:61"));
    }

    SECTION("Formatting") {
      std::string ten_sec = format_time_mm_ss(SECOND * 10);
      REQUIRE(ten_sec == "00:10");

      std::string ten_half_sec = format_time_mm_ss(SECOND * 10.5);
      REQUIRE(ten_half_sec == "00:10");

      std::string min_format = format_time_mm_ss(ONE_MINUTE);
      REQUIRE(min_format == "01:00");

      std::string min_half_format = format_time_mm_ss(ONE_MINUTE + HALF_MINUTE);
      REQUIRE(min_half_format == "01:30");

      std::string hour_format = format_time_mm_ss(ONE_HOUR);
      REQUIRE(hour_format == "60:00");

      std::string hour_min_30_format = format_time_mm_ss(ONE_HOUR + ONE_MINUTE + HALF_MINUTE);
      REQUIRE(hour_min_30_format == "61:30");

      std::string seventy_hours_format = format_time_mm_ss(ONE_HOUR * 70);
      REQUIRE(seventy_hours_format == "4200:00");
    }

    SECTION("Parsing") {
      REQUIRE(parse_m_ss_duration("01:30") == 90);
      REQUIRE(parse_m_ss_duration("00:30") == 30);
      REQUIRE(parse_m_ss_duration("00:00") == 00);
      REQUIRE(parse_m_ss_duration("02:00") == 120);
      REQUIRE(parse_m_ss_duration("30:00") == 1800);
      REQUIRE(parse_m_ss_duration("30:35") == 1835);

      REQUIRE(parse_m_ss_duration("1:30") == 90);
      REQUIRE(parse_m_ss_duration("0:30") == 30);
      REQUIRE(parse_m_ss_duration("0:00") == 00);
      REQUIRE(parse_m_ss_duration("2:00") == 120);
    }

  } // SECTION mm:ss

  SECTION("Format duration") {
    std::string ten_sec = format_duration(SECOND * 10);
    REQUIRE(ten_sec == "00:10");

    std::string ten_half_sec = format_duration(SECOND * 10.5);
    REQUIRE(ten_sec == "00:10");

    std::string min_format = format_duration(ONE_MINUTE);
    REQUIRE(min_format == "01:00");

    std::string min_half_format = format_duration(ONE_MINUTE + HALF_MINUTE);
    REQUIRE(min_half_format == "01:30");

    std::string almost_hour_format = format_duration(ONE_HOUR - 1);
    REQUIRE(almost_hour_format == "59:59");

    std::string hour_format = format_duration(ONE_HOUR);
    REQUIRE(hour_format == "01:00:00");

    std::string hour_min_30_format = format_duration(ONE_HOUR + ONE_MINUTE + HALF_MINUTE);
    REQUIRE(hour_min_30_format == "01:01:30");

    std::string hour_half_30_format = format_duration(ONE_HOUR + HALF_HOUR + HALF_MINUTE);
    REQUIRE(hour_half_30_format == "01:30:30");


    std::string seventy_hours_format = format_duration(ONE_HOUR * 70);
    REQUIRE(seventy_hours_format == "70:00:00");
  }

  SECTION("trim") {
    #define TRIM_TEST(str, trimchars, res) \
      REQUIRE(str_trim(str, trimchars) == res); \
      REQUIRE(strv_trim(str, trimchars) == res);
    
    TRIM_TEST("    Trimmed   String       ", " ", "Trimmed   String");
    TRIM_TEST("    Trimmed   String       ", " ", "Trimmed   String");
    TRIM_TEST("Trimmed   String       ", " ", "Trimmed   String");
    TRIM_TEST("    Trimmed   String", " ", "Trimmed   String");
    TRIM_TEST(" www  w Trimmed ww String       wwwwww", " w", "Trimmed ww String");
    TRIM_TEST("    Trimmed   String       \r\n", " \r\n", "Trimmed   String");
    TRIM_TEST(" _ !! w !Trimmed !__w_!  String_w   __w    ", " w_!", "Trimmed !__w_!  String");
  }

  SECTION("strsplit") {
    #define STRSPLIT_TEST(str, splitchar, ...) \
      REQUIRE(are_vectors_equal(strsplit(str, splitchar), __VA_ARGS__)); \
      REQUIRE(are_vectors_equal(strvsplit(str, splitchar), __VA_ARGS__)); 
    
    STRSPLIT_TEST("this,is,split", ',', {"this", "is", "split"});
    STRSPLIT_TEST("this,is,split", ',', {"this", "is", "split"});
    STRSPLIT_TEST("this,,is,,,split,,,", ',', {"this", "is", "split"});
    STRSPLIT_TEST(",this,is,,,split", ',', {"this", "is", "split"});
    STRSPLIT_TEST(",,,,,", ',', {});
    STRSPLIT_TEST("", ',', {});
    STRSPLIT_TEST("  ", ',', {"  "});
  }
}