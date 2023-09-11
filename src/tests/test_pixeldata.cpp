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
    

    REQUIRE(grayscale.at(0, 0).is_grayscale());
    REQUIRE(grayscale.at(0, 0).red == 255);
    REQUIRE(grayscale.at(0, 0).green == 255);
    REQUIRE(grayscale.at(0, 0).blue == 255);
    REQUIRE(grayscale.at(0, 2).red == 120);
    REQUIRE(grayscale.at(0, 2).green == 120);
    REQUIRE(grayscale.at(0, 2).blue == 120);
    REQUIRE(grayscale.at(0, 2).blue == 120);
    REQUIRE(grayscale.at(0, 2).is_grayscale());

    REQUIRE(grayscale.get_width() == 6);
    REQUIRE(grayscale.get_height() == 8);
  }

  SECTION("copied") {
    PixelData copied = grayscale;
    REQUIRE(copied.equals(grayscale));
  }

  SECTION("bounded - same size") {
    PixelData bounded = grayscale.bound(6, 8);
    REQUIRE(bounded.equals(grayscale));
  }

  SECTION("bounded - bigger size") {
    PixelData bounded = grayscale.bound(10, 10);
    REQUIRE(bounded.equals(grayscale));
  }

  SECTION("scale - bigger size") {
    PixelData bounded = grayscale.scale(2.0);
    REQUIRE(bounded.get_width() == grayscale.get_width() * 2);
    REQUIRE(bounded.get_height() == grayscale.get_height() * 2);
  }

  SECTION("scale - same size") {
    PixelData bounded = grayscale.scale(1.0);
    REQUIRE(bounded.get_width() == grayscale.get_width());
    REQUIRE(bounded.get_height() == grayscale.get_height());
  }

  SECTION("scale - smaller size") {
    PixelData bounded = grayscale.scale(0.5);
    REQUIRE(bounded.get_width() == grayscale.get_width() / 2);
    REQUIRE(bounded.get_height() == grayscale.get_height() / 2);
  }

}