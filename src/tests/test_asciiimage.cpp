#include "ascii.h"
#include "pixeldata.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("asciiimage", "[image manipulation]") {
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


  

}