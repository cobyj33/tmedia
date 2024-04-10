#include "color.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("color", "[color]") {

  SECTION("RGB24 Average") {
    RGB24 first(10, 20, 30);
    RGB24 second(20, 30, 40);
    RGB24 third(30, 40, 50);
    RGB24 fourth(40, 50, 60);
    RGB24 fifth(50, 60, 70);
    std::vector<RGB24> colors{first, second, third, fourth, fifth};
    RGB24 average = get_average_color(colors);
    REQUIRE(average.r == 30);
    REQUIRE(average.g == 40);
    REQUIRE(average.b == 50);
  }

}