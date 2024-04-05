#include "color.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("color", "[color]") {

  SECTION("Grayscale Constructor") {
    RGB24 color(10);

    REQUIRE(color.r == 10);
    REQUIRE(color.g == 10);
    REQUIRE(color.b == 10);
  }

  SECTION("RGB Constructor") {
    RGB24 color(10, 20, 30);

    REQUIRE(color.r == 10);
    REQUIRE(color.g == 20);
    REQUIRE(color.b == 30);
  }

  SECTION("RGB24 Average") {
    RGB24 first(10);
    RGB24 second(20);
    std::vector<RGB24> colors{first, second};
    RGB24 average = get_average_color(colors);
    REQUIRE(average.r == 15);
    REQUIRE(average.g == 15);
    REQUIRE(average.b == 15);
  }

}