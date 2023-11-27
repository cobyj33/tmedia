#include "palette.h"
#include "palette_io.h"

#include <catch2/catch_test_macros.hpp>

const char* test1 = "GIMP Palette\n"
"Channels: RGBA\n"
"#\n"
"  0   0   0   0 Transparent\n"
"254  91  89 255 Red\n"
"247 165  71 255 Orange\n"
"243 206  82 255 Yellow\n"
"106 205  91 255 Green\n"
" 87 185 242 255 Blue\n"
"209 134 223 255 Purple\n"
"165 165 167 255 Gray\n";

Palette test1Palette{
  RGBColor(0, 0, 0),
  RGBColor(254, 91, 89),
  RGBColor(247, 165, 71),
  RGBColor(243, 206, 82),
  RGBColor(106, 205, 91),
  RGBColor(87, 185, 242),
  RGBColor(209, 134, 223),
  RGBColor(165, 165, 167),
};


TEST_CASE("palette", "[palette]") {

  SECTION("gpl", "[gpl]") {
    SECTION("test1") {
      REQUIRE_NOTHROW([] () {
        Palette palette = read_palette_str(test1);
      });
      Palette palette = read_palette_str(test1);
      REQUIRE(palette_equals(palette, test1Palette)); 
    }

  }

}