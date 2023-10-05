#include "audiobuffer.h"
#include <cmath>
#include <cstdlib>


#include <catch2/catch_test_macros.hpp>

float* get_mock_samples(int nb_channels, int nb_samples) {
  float* samples = (float*)std::malloc(sizeof(float) * nb_channels * nb_samples);
  for (int i = 0; i < nb_channels * nb_samples; i++) {
    samples[i] = std::sin(double(i) / 1000);
  }
  return samples;
}

TEST_CASE("Audio buffer", "[structure]") {
  const int NB_CHANNELS = 2;
  const int SAMPLE_RATE = 44100;
  const double EXPECTED_BUFFER_TIME = 20;

  AudioBuffer audio_buffer(2, 44100);

  REQUIRE_FALSE( audio_buffer.can_read(1) );
  REQUIRE( audio_buffer.get_nb_samples() == 0 );
  REQUIRE( audio_buffer.get_elapsed_time() == 0.0 );

  SECTION("Initialization") {
    REQUIRE_FALSE( audio_buffer.can_read(1) );
    REQUIRE( audio_buffer.get_nb_samples() == 0 );
    REQUIRE( audio_buffer.get_elapsed_time() == 0 );
    REQUIRE( audio_buffer.get_nb_channels() == 2 );
    REQUIRE( audio_buffer.get_sample_rate() == 44100 );
  }

  SECTION("Reading and Writing") {
    const int NB_CHANNELS = 2;
    const int SAMPLE_RATE = 44100;
    const double EXPECTED_BUFFER_TIME = 20;

    int expected_nb_samples = SAMPLE_RATE * EXPECTED_BUFFER_TIME;
    REQUIRE(audio_buffer.get_time() == 0.0);
    float* mock_samples = get_mock_samples(2, expected_nb_samples);
    audio_buffer.write(mock_samples, expected_nb_samples);

    REQUIRE(audio_buffer.get_nb_samples() == (std::size_t)expected_nb_samples);
    REQUIRE(audio_buffer.get_time() == 0.0);
    REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
    REQUIRE(audio_buffer.get_end_time() == EXPECTED_BUFFER_TIME);
    REQUIRE(audio_buffer.can_read());
    REQUIRE(audio_buffer.get_nb_can_read() == (std::size_t)expected_nb_samples);
    REQUIRE(audio_buffer.can_read(audio_buffer.get_nb_can_read()));

    SECTION("Multiple Consecutive Writes") { // Audio buffer already has mock_samples written into it
      audio_buffer.write(mock_samples, expected_nb_samples);
      REQUIRE(audio_buffer.get_nb_channels() == NB_CHANNELS);
      REQUIRE(audio_buffer.get_sample_rate() == SAMPLE_RATE);
      REQUIRE(audio_buffer.get_nb_samples() == (std::size_t)expected_nb_samples * 2);
      REQUIRE(audio_buffer.get_time() == 0.0);
      REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
      REQUIRE(audio_buffer.get_end_time() == EXPECTED_BUFFER_TIME * 2);
      REQUIRE(audio_buffer.can_read());
      REQUIRE(audio_buffer.get_nb_can_read() == (std::size_t)expected_nb_samples * 2);
    }

    SECTION("Clear and restart") { // Audio buffer already has mock_samples written into it
      audio_buffer.clear_and_restart_at(0);
      audio_buffer.write(mock_samples, expected_nb_samples);
      REQUIRE(audio_buffer.get_nb_channels() == NB_CHANNELS);
      REQUIRE(audio_buffer.get_sample_rate() == SAMPLE_RATE);
      REQUIRE(audio_buffer.get_nb_samples() == (std::size_t)expected_nb_samples);
      REQUIRE(audio_buffer.get_time() == 0.0);
      REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
      REQUIRE(audio_buffer.get_end_time() == EXPECTED_BUFFER_TIME);
      REQUIRE(audio_buffer.can_read());
      REQUIRE(audio_buffer.get_nb_can_read() == (std::size_t)expected_nb_samples);
    }

    SECTION("Clear and restart with time") { // Audio buffer already has mock_samples written into it
      const int start_time = 10.0;
      audio_buffer.clear_and_restart_at(start_time);
      audio_buffer.write(mock_samples, expected_nb_samples);
      REQUIRE(audio_buffer.get_nb_channels() == NB_CHANNELS);
      REQUIRE(audio_buffer.get_sample_rate() == SAMPLE_RATE);
      REQUIRE(audio_buffer.get_nb_samples() == (std::size_t)expected_nb_samples);
      REQUIRE(audio_buffer.get_time() == start_time);
      REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
      REQUIRE(audio_buffer.get_end_time() == (std::size_t)EXPECTED_BUFFER_TIME + start_time);
      REQUIRE(audio_buffer.can_read());
      REQUIRE(audio_buffer.get_nb_can_read() == (std::size_t)expected_nb_samples);
    }

    SECTION("Clear and restart after write with time") { // Audio buffer already has mock_samples written into it
      const int start_time = 10.0;
      audio_buffer.write(mock_samples, expected_nb_samples);

      REQUIRE(audio_buffer.get_nb_channels() == NB_CHANNELS);
      REQUIRE(audio_buffer.get_sample_rate() == SAMPLE_RATE);

      audio_buffer.clear_and_restart_at(start_time);

      REQUIRE(audio_buffer.get_nb_channels() == NB_CHANNELS);
      REQUIRE(audio_buffer.get_sample_rate() == SAMPLE_RATE);

      audio_buffer.write(mock_samples, expected_nb_samples);

      REQUIRE(audio_buffer.get_nb_channels() == NB_CHANNELS);
      REQUIRE(audio_buffer.get_sample_rate() == SAMPLE_RATE);

      REQUIRE(audio_buffer.get_nb_samples() == (std::size_t)expected_nb_samples);
      REQUIRE(audio_buffer.get_time() == start_time);
      REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
      REQUIRE(audio_buffer.get_end_time() == EXPECTED_BUFFER_TIME + start_time);
      REQUIRE(audio_buffer.can_read());
      REQUIRE(audio_buffer.get_nb_can_read() == (std::size_t)expected_nb_samples);
    }

    SECTION("Clear") { // Audio buffer already has mock_samples written into it
      audio_buffer.clear();
      REQUIRE(audio_buffer.get_nb_channels() == NB_CHANNELS);
      REQUIRE(audio_buffer.get_sample_rate() == SAMPLE_RATE);

      REQUIRE(audio_buffer.get_nb_samples() == 0);
      REQUIRE(audio_buffer.get_time() == 0);
      REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
      REQUIRE(audio_buffer.get_end_time() == 0);
      REQUIRE_FALSE(audio_buffer.can_read());
      REQUIRE_FALSE(audio_buffer.can_read(1));
      REQUIRE(audio_buffer.get_nb_can_read() == 0);
    }

    SECTION("Set time") { // Audio buffer already has mock_samples written into it
      const double time_to_set = EXPECTED_BUFFER_TIME / 2;
      audio_buffer.set_time_in_bounds(time_to_set);
      REQUIRE(audio_buffer.get_nb_channels() == NB_CHANNELS);
      REQUIRE(audio_buffer.get_sample_rate() == SAMPLE_RATE);

      REQUIRE(audio_buffer.get_elapsed_time() == time_to_set);
      REQUIRE(audio_buffer.get_end_time() == EXPECTED_BUFFER_TIME);
      REQUIRE(audio_buffer.get_time() == time_to_set);
      REQUIRE(audio_buffer.get_nb_can_read() == (std::size_t)expected_nb_samples / 2);
      REQUIRE(audio_buffer.can_read());
      REQUIRE(audio_buffer.can_read(1));
      REQUIRE(audio_buffer.get_nb_samples() == (std::size_t)expected_nb_samples);
    }

    SECTION("Writing one sample") {
      AudioBuffer audio_buffer(NB_CHANNELS, SAMPLE_RATE);
      float sample[2] = {0.0, 1.0};
      audio_buffer.write(sample, 1);
      REQUIRE(audio_buffer.can_read(1));
      REQUIRE(audio_buffer.can_read());
      REQUIRE(audio_buffer.get_nb_can_read() == 1);
      REQUIRE(audio_buffer.can_read(audio_buffer.get_nb_can_read()));
    }

    SECTION("Writing two samples") {
      AudioBuffer audio_buffer(NB_CHANNELS, SAMPLE_RATE);
      float sample[4] = {0.0, 1.0, 1.0, 0.0};
      audio_buffer.write(sample, 2);
      REQUIRE(audio_buffer.can_read(2));
      REQUIRE(audio_buffer.can_read(1));
      REQUIRE(audio_buffer.can_read());
      REQUIRE(audio_buffer.get_nb_can_read() == 2);
      REQUIRE(audio_buffer.can_read(audio_buffer.get_nb_can_read()));
    }

    std::free(mock_samples);
  } // "Reading and Writing"

  SECTION("leave behind") {
    AudioBuffer audio_buffer(2, 44100);
    float* mock_samples = get_mock_samples(2, 44100 * 60);
    audio_buffer.write(mock_samples, 44100 * 60);

    audio_buffer.set_time_in_bounds(50);
    REQUIRE(audio_buffer.get_elapsed_time() == 50.0);
    REQUIRE(audio_buffer.get_time() == 50.0);
    REQUIRE(audio_buffer.get_nb_samples() == 44100 * 60);

    audio_buffer.leave_behind(15);
    REQUIRE(audio_buffer.get_elapsed_time() == 15.0);
    REQUIRE(audio_buffer.get_time() == 50.0);
    REQUIRE(audio_buffer.get_nb_samples() == 44100 * 25);


    std::free(mock_samples);
  }


}