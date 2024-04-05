#include "scale.h"

#include <utility>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Scaling functions", "[image manipulation]") {
  int width = 100;
  int height = 100;

  SECTION("get scale factor double") {
    double scale_factor = get_scale_factor(width, height, width * 2, height * 2);
    REQUIRE(scale_factor == 2); 
  }

  SECTION("get scale factor half") {
    double scale_factor = get_scale_factor(width, height, width / 2, height / 2);
    REQUIRE(scale_factor == 0.5); 
  }

  SECTION("get scale double - same aspect ratio") {
    Dim2 scaled = get_scale_size(width, height, width * 2, height * 2);
    REQUIRE(scaled.width == width * 2);
    REQUIRE(scaled.height == height * 2);
  }

  SECTION("get scale double - bigger aspect ratio but still fits in new bounds") {
    Dim2 scaled = get_scale_size(width, height, width * 2, height * 3);
    REQUIRE(scaled.width == width * 2);
    REQUIRE(scaled.height == height * 2);
  }

  SECTION("get scale double - one dimension stays the same (height)") {
    Dim2 scaled = get_scale_size(width, height, width * 2, height);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }

  SECTION("get scale double - one dimension stays the same (width)") {
    Dim2 scaled = get_scale_size(width, height, width, height * 2);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }

  SECTION("get bounded dimensions - bounds doubled") {
    Dim2 scaled = bound_dims(width, height, width * 2, height * 2);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }

  SECTION("get bounded dimensions - bounds halved") {
    Dim2 scaled = bound_dims(width, height, width / 2, height / 2);
    REQUIRE(scaled.width == width / 2);
    REQUIRE(scaled.height == height / 2);
  }

  SECTION("get bounded dimensions - one dimension increases (width)") {
    Dim2 scaled = bound_dims(width, height, width * 2, height);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }

  SECTION("get bounded dimensions - one dimension increases (height)") {
    Dim2 scaled = bound_dims(width, height, width, height * 2);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }



}