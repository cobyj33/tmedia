#include "pixeldata.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("pixeldata", "[image manipulation]") {

    SECTION("grayscale") {
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
        PixelData pixeldata(grayscale_test);

        REQUIRE(pixeldata.at(0, 0).is_grayscale());
        REQUIRE(pixeldata.at(0, 0).red == 255);
        REQUIRE(pixeldata.at(0, 0).green == 255);
        REQUIRE(pixeldata.at(0, 0).blue == 255);
        REQUIRE(pixeldata.at(0, 2).red == 120);
        REQUIRE(pixeldata.at(0, 2).green == 120);
        REQUIRE(pixeldata.at(0, 2).blue == 120);
        REQUIRE(pixeldata.at(0, 2).blue == 120);

        REQUIRE(pixeldata.get_width() == 6);
        REQUIRE(pixeldata.get_height() == 8);
    }


}