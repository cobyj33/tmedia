#include "pixeldata.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("pixeldata", "[image manipulation]") {
  std::vector<std::vector<uint8_t>> grayscale_test{
      { 255, 255, 120, 120, 255, 255 },
      { 255, 255, 120, 120, 255, 255 },
      { 255, 255, 120, 120, 255, 255 },
      { 255, 255, 120, 120, 255, 255 },
      { 255, 255, 120, 120, 255, 255 },
      { 255, 255, 120, 120, 255, 255 },
      { 255, 255, 120, 120, 255, 255 },
      { 255, 255, 120, 120, 255, 255 }
    };
  PixelData grayscale(grayscale_test);

  SECTION("grayscale") {
    

    REQUIRE(grayscale.at(0, 0).is_gray());
    REQUIRE(grayscale.at(0, 0).r == 255);
    REQUIRE(grayscale.at(0, 0).g == 255);
    REQUIRE(grayscale.at(0, 0).b == 255);
    REQUIRE(grayscale.at(0, 2).r == 120);
    REQUIRE(grayscale.at(0, 2).g == 120);
    REQUIRE(grayscale.at(0, 2).b == 120);
    REQUIRE(grayscale.at(0, 2).b == 120);
    REQUIRE(grayscale.at(0, 2).is_gray());

    REQUIRE(grayscale.get_width() == 6);
    REQUIRE(grayscale.get_height() == 8);
  }

  SECTION("copied") {
    PixelData copied = grayscale;
    REQUIRE(copied.equals(grayscale));
  }

  SECTION("bounded - same size") {
    PixelData bounded = grayscale.bound(6, 8, ScalingAlgo::BOX_SAMPLING);
    REQUIRE(bounded.equals(grayscale));
  }

  SECTION("bounded - bigger size") {
    PixelData bounded = grayscale.bound(10, 10, ScalingAlgo::BOX_SAMPLING);
    REQUIRE(bounded.equals(grayscale));
  }

  SECTION("scale - bigger size") {
    PixelData bounded = grayscale.scale(2.0, ScalingAlgo::BOX_SAMPLING);
    REQUIRE(bounded.get_width() == grayscale.get_width() * 2);
    REQUIRE(bounded.get_height() == grayscale.get_height() * 2);
  }

  SECTION("scale - same size") {
    PixelData bounded = grayscale.scale(1.0, ScalingAlgo::BOX_SAMPLING);
    REQUIRE(bounded.get_width() == grayscale.get_width());
    REQUIRE(bounded.get_height() == grayscale.get_height());
  }

  SECTION("scale - smaller size") {
    PixelData bounded = grayscale.scale(0.5, ScalingAlgo::BOX_SAMPLING);
    REQUIRE(bounded.get_width() == grayscale.get_width() / 2);
    REQUIRE(bounded.get_height() == grayscale.get_height() / 2);
  }

}