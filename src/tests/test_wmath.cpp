#include "wmath.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Testing Wrapper Math Functions", "[wrapper]") {
    SECTION("clamp") {
        REQUIRE( clamp(2, 0, 5) == 2 );
        REQUIRE( clamp(-1, 0, 5) == 0 );
        REQUIRE( clamp(6, 0, 5) == 5  );
        REQUIRE( clamp(5, 0, 5) == 5);
        REQUIRE( clamp(0, 0, 5) == 0);
        
        REQUIRE( clamp(2, 5, 0) == 2 );
        REQUIRE( clamp(-1, 5, 0) == 0 );
        REQUIRE( clamp(6, 5, 0) == 5  );
        REQUIRE( clamp(5, 5, 0) == 5);
        REQUIRE( clamp(0, 5, 0) == 0);
    }

    SECTION("signum") {
        REQUIRE( signum(0.5) == 1 );
        REQUIRE( signum(134.542) == 1 );
        REQUIRE( signum(-0.5) == -1 );
        REQUIRE( signum(-42.333213) == -1 );
        REQUIRE( signum(0) == 0 );
        REQUIRE( signum(2) == 1 );
        REQUIRE( signum(35897) == 1 );
        REQUIRE( signum(-2) == -1 );
        REQUIRE( signum(-53623) == -1 ); 
        REQUIRE( signum(0) == 0 ); 
    }



}