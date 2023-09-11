#include "playback.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("playback", "[playback]") {
  const int MOCK_SYSTEM_START_TIME = 100;
  Playback playback;
  playback.start(MOCK_SYSTEM_START_TIME);
  REQUIRE(playback.get_speed() == 1.0);
  REQUIRE(playback.get_volume() == 1.0);
  REQUIRE(playback.get_time(MOCK_SYSTEM_START_TIME) == 0);
  REQUIRE(playback.is_playing());
  
  SECTION("Play") {
    REQUIRE(playback.get_time(MOCK_SYSTEM_START_TIME + 10) == 10);

    SECTION("Skip") {
      REQUIRE(playback.is_playing());
      playback.skip(10);
      REQUIRE(playback.get_time(MOCK_SYSTEM_START_TIME + 10) == 20);
      playback.skip(-10);
      REQUIRE(playback.get_time(MOCK_SYSTEM_START_TIME + 10) == 10);
      playback.skip(20);
      REQUIRE(playback.get_time(MOCK_SYSTEM_START_TIME + 10) == 30);
      playback.skip(20);
      REQUIRE(playback.get_time(MOCK_SYSTEM_START_TIME + 10) == 50);
    }
    

    SECTION("Pause") {
      playback.stop(MOCK_SYSTEM_START_TIME + 10); // pause for 10 seconds
      playback.resume(MOCK_SYSTEM_START_TIME + 20);
      REQUIRE(playback.get_time(MOCK_SYSTEM_START_TIME + 20) == 10); //Time should be the same after 20 second pause
    }

  }
}