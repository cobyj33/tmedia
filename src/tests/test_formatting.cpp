#include "formatting.h"

#include <catch2/catch_test_macros.hpp>

constexpr double HALF_SECOND = 0.5;
constexpr int SECOND = 1;
constexpr int TEN_SECONDS = 10;
constexpr int HALF_MINUTE = 30;
constexpr int ONE_MINUTE = 60;
constexpr int HALF_HOUR = 1800;
constexpr int ONE_HOUR = 3600;


TEST_CASE("Formatting", "[functions]") {

    SECTION("double to fixed string") {
        REQUIRE(double_to_fixed_string(1.234, 2) == "1.23");
        REQUIRE(double_to_fixed_string(1.23456, 2) == "1.23");
        REQUIRE(double_to_fixed_string(1.23456, 8) == "1.23456000");
        REQUIRE(double_to_fixed_string(1.23456, 1) == "1.2");
        REQUIRE(double_to_fixed_string(34543.323, 2) == "34543.32");
        REQUIRE(double_to_fixed_string(34543.323, 1) == "34543.3");
        REQUIRE(double_to_fixed_string(34543.323, 4) == "34543.3230");
        REQUIRE(double_to_fixed_string(34543.323, 6) == "34543.323000");

        REQUIRE(double_to_fixed_string(34543.323, 0) == "34543");
        REQUIRE(double_to_fixed_string(-34543.323, 0) == "-34543");
        REQUIRE(double_to_fixed_string(100, 0) == "100");
        REQUIRE(double_to_fixed_string(100, 2) == "100.00");

        REQUIRE_THROWS(double_to_fixed_string(100, -1));
    }
    

    SECTION("hh:mm:ss") {
        SECTION("validation") {
            REQUIRE(is_hh_mm_ss_duration("00:00:10"));
            REQUIRE(is_hh_mm_ss_duration("00:01:10"));
            REQUIRE(is_hh_mm_ss_duration("00:10:10"));
            REQUIRE(is_hh_mm_ss_duration("00:01:00"));
            REQUIRE(is_hh_mm_ss_duration("00:01:30"));
            REQUIRE(is_hh_mm_ss_duration("01:00:10"));
            REQUIRE(is_hh_mm_ss_duration("01:01:10"));
            REQUIRE(is_hh_mm_ss_duration("70:00:10"));
        }

        SECTION("Incorrect") {
            REQUIRE_FALSE(is_hh_mm_ss_duration("00:01"));
            REQUIRE_FALSE(is_hh_mm_ss_duration(":00:00:01"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("00:00:01:"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("00:0.1"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("00:01."));
            REQUIRE_FALSE(is_hh_mm_ss_duration("0.0:01"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("0:0:0"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("1:1:1"));

            REQUIRE_FALSE(is_hh_mm_ss_duration("10:00:61"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("10:61:00"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("10:61:61"));

            REQUIRE_FALSE(is_hh_mm_ss_duration("10:-34:15"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("10:34:-15"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("-10:34:15"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("10 34 15"));
            REQUIRE_FALSE(is_hh_mm_ss_duration("-10 34 15"));
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
            REQUIRE(is_mm_ss_duration("00:10"));
            REQUIRE(is_mm_ss_duration("01:10"));
            REQUIRE(is_mm_ss_duration("10:10"));
            REQUIRE(is_mm_ss_duration("01:00"));
            REQUIRE(is_mm_ss_duration("01:30"));
            REQUIRE(is_mm_ss_duration("00:10"));
            REQUIRE(is_mm_ss_duration("01:10"));
            REQUIRE(is_mm_ss_duration("61:10"));
        }

        SECTION("Invalid") {
            REQUIRE_FALSE(is_mm_ss_duration("00:61"));
            REQUIRE_FALSE(is_mm_ss_duration("00:99"));
            REQUIRE_FALSE(is_mm_ss_duration("00:10:"));
            REQUIRE_FALSE(is_mm_ss_duration(":00:10"));
            REQUIRE_FALSE(is_mm_ss_duration("000:10"));
            REQUIRE_FALSE(is_mm_ss_duration("00:00:10"));
            
            REQUIRE_FALSE(is_mm_ss_duration("-00:10"));
            REQUIRE_FALSE(is_mm_ss_duration("-000:10"));
            REQUIRE_FALSE(is_mm_ss_duration("-00:61"));
        }

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
}