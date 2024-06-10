#include <tmedia/media/mediaclock.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("mediaclock", "[mediaclock]") {
  const int MOCK_SYSTEM_START_TIME = 100;
  MediaClock clock;
  clock.init(MOCK_SYSTEM_START_TIME);
  REQUIRE(clock.get_time(MOCK_SYSTEM_START_TIME) == 0);
  REQUIRE(clock.is_playing());
  
  SECTION("Play") {
    REQUIRE(clock.get_time(MOCK_SYSTEM_START_TIME + 10) == 10);

    SECTION("Skip") {
      REQUIRE(clock.is_playing());
      clock.skip(10);
      REQUIRE(clock.get_time(MOCK_SYSTEM_START_TIME + 10) == 20);
      clock.skip(-10);
      REQUIRE(clock.get_time(MOCK_SYSTEM_START_TIME + 10) == 10);
      clock.skip(20);
      REQUIRE(clock.get_time(MOCK_SYSTEM_START_TIME + 10) == 30);
      clock.skip(20);
      REQUIRE(clock.get_time(MOCK_SYSTEM_START_TIME + 10) == 50);
    }
    

    SECTION("Pause") {
      clock.stop(MOCK_SYSTEM_START_TIME + 10); // pause for 10 seconds
      clock.resume(MOCK_SYSTEM_START_TIME + 20);
      REQUIRE(clock.get_time(MOCK_SYSTEM_START_TIME + 20) == 10); //Time should be the same after 20 second pause
    }

    SECTION("Get Time in Pause") {
      clock.stop(MOCK_SYSTEM_START_TIME + 10);
      REQUIRE(clock.get_time(MOCK_SYSTEM_START_TIME + 20) == 10); //calling 
    }

  }
}
