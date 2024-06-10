#include <tmedia/image/color.h>
#include <iostream>

// https://github.com/catchorg/Catch2/blob/devel/docs/tostring.md
std::ostream& operator << ( std::ostream& os, RGB24 const& value ) {
  // static casts so uint8_t is not interpreted as char
  os << "{ " << static_cast<int>(value.r) << ", " << static_cast<int>(value.g)
    << ", " << static_cast<int>(value.b) << " }";
  return os;
}

#include <catch2/catch_test_macros.hpp>

TEST_CASE("color", "[color]") {

  SECTION("RGB24 Literals") {
    REQUIRE( 0x000000_rgb == RGB24(0, 0, 0) );
    REQUIRE( 0xff0000_rgb == RGB24(255, 0, 0) );
    REQUIRE( 0x00ff00_rgb == RGB24(0, 255, 0) );
    REQUIRE( 0x0000ff_rgb == RGB24(0, 0, 255) );
    REQUIRE( 0xffff00_rgb == RGB24(255, 255, 0) );
    REQUIRE( 0x00ffff_rgb == RGB24(0, 255, 255) );
    REQUIRE( 0xff00ff_rgb == RGB24(255, 0, 255) );
    REQUIRE( 0xffffff_rgb == RGB24(255, 255, 255) );

    REQUIRE( 0x800000_rgb == RGB24(128, 0, 0) );
    REQUIRE( 0xc0c0c0_rgb == RGB24(192, 192, 192));
    REQUIRE( 0x00875f_rgb == RGB24(0, 135, 95));
    REQUIRE( 0x5f87d7_rgb == RGB24(95, 135, 215));
  }
}
