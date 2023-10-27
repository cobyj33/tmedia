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
    VideoDimensions scaled = get_scale_size(width, height, width * 2, height * 2);
    REQUIRE(scaled.width == width * 2);
    REQUIRE(scaled.height == height * 2);
  }

  SECTION("get scale double - bigger aspect ratio but still fits in new bounds") {
    VideoDimensions scaled = get_scale_size(width, height, width * 2, height * 3);
    REQUIRE(scaled.width == width * 2);
    REQUIRE(scaled.height == height * 2);
  }

  SECTION("get scale double - one dimension stays the same (height)") {
    VideoDimensions scaled = get_scale_size(width, height, width * 2, height);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }

  SECTION("get scale double - one dimension stays the same (width)") {
    VideoDimensions scaled = get_scale_size(width, height, width, height * 2);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }

  SECTION("get bounded dimensions - bounds doubled") {
    VideoDimensions scaled = get_bounded_dimensions(width, height, width * 2, height * 2);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }

  SECTION("get bounded dimensions - bounds halved") {
    VideoDimensions scaled = get_bounded_dimensions(width, height, width / 2, height / 2);
    REQUIRE(scaled.width == width / 2);
    REQUIRE(scaled.height == height / 2);
  }

  SECTION("get bounded dimensions - one dimension increases (width)") {
    VideoDimensions scaled = get_bounded_dimensions(width, height, width * 2, height);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }

  SECTION("get bounded dimensions - one dimension increases (height)") {
    VideoDimensions scaled = get_bounded_dimensions(width, height, width, height * 2);
    REQUIRE(scaled.width == width);
    REQUIRE(scaled.height == height);
  }



}