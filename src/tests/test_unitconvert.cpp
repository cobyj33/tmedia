#include <tmedia/util/unitconvert.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Unit Conversion Constants", "[constants]") {

  REQUIRE(30 * SECONDS_TO_MINUTES == 0.5);
  REQUIRE(60 * SECONDS_TO_MINUTES == 1.0);
  REQUIRE(2 * MINUTES_TO_SECONDS == 120);

  REQUIRE(2 * HOURS_TO_SECONDS == 7200);
  REQUIRE(60 * SECONDS_TO_HOURS == 1.0 / 60);

  REQUIRE(2 * SECONDS_TO_NANOSECONDS == 2000000000);
  REQUIRE(2 * MILLISECONDS_TO_NANOSECONDS == 2000000);

}