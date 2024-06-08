#include <tmedia/util/wmath.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Testing Wrapper Math Functions", "[wrapper]") {
  SECTION("clamp") {
    REQUIRE( clamp<int>(2, 0, 5) == 2 );
    REQUIRE( clamp<int>(-1, 0, 5) == 0 );
    REQUIRE( clamp<int>(6, 0, 5) == 5  );
    REQUIRE( clamp<int>(5, 0, 5) == 5);
    REQUIRE( clamp<int>(0, 0, 5) == 0);
    
    REQUIRE( clamp<int>(2, 5, 0) == 2 );
    REQUIRE( clamp<int>(-1, 5, 0) == 0 );
    REQUIRE( clamp<int>(6, 5, 0) == 5  );
    REQUIRE( clamp<int>(5, 5, 0) == 5);
    REQUIRE( clamp<int>(0, 5, 0) == 0);
  }

  SECTION("In range") {
    REQUIRE(in_range(10, 5, 15));
    REQUIRE(in_range(5, 5, 15));
    REQUIRE(in_range(15, 5, 15));

    REQUIRE_FALSE(in_range(16, 5, 15));
    REQUIRE_FALSE(in_range(4, 5, 15));
    REQUIRE_FALSE(in_range(-4, 5, 15));
    REQUIRE_FALSE(in_range(-16, 5, 15));
    REQUIRE_FALSE(in_range(0, 5, 15));

    REQUIRE(in_range(10, 15, 5));
    REQUIRE(in_range(5, 15, 5));
    REQUIRE(in_range(15, 15, 5));

    REQUIRE_FALSE(in_range(16, 15, 5));
    REQUIRE_FALSE(in_range(4, 15, 5));
    REQUIRE_FALSE(in_range(-4, 15, 5));
    REQUIRE_FALSE(in_range(-16, 15, 5));
    REQUIRE_FALSE(in_range(0, 15, 5));
  }



}