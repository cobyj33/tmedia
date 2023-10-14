#include "color.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("color", "[color]") {

  SECTION("Grayscale Constructor") {
    RGBColor color(10);

    REQUIRE(color.red == 10);
    REQUIRE(color.green == 10);
    REQUIRE(color.blue == 10);
  }

  SECTION("RGB Constructor") {
    RGBColor color(10, 20, 30);

    REQUIRE(color.red == 10);
    REQUIRE(color.green == 20);
    REQUIRE(color.blue == 30);
  }

  SECTION("RGBColor Average") {
    RGBColor first(10);
    RGBColor second(20);
    std::vector<RGBColor> colors{first, second};
    RGBColor average = get_average_color(colors);
    REQUIRE(average.red == 15);
    REQUIRE(average.green == 15);
    REQUIRE(average.blue == 15);
  }

}