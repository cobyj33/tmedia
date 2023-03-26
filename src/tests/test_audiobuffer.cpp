#include "audiobuffer.h"
#include <cmath>
#include <cstdlib>


#include <catch2/catch_test_macros.hpp>

constexpr double PI  = 3.14159265359;

float* get_mock_samples(int nb_channels, int nb_samples) {
    float* samples = (float*)std::malloc(sizeof(float) * nb_channels * nb_samples);
    for (int i = 0; i < nb_channels * nb_samples; i++) {
        samples[i] = std::sin(double(i) / 1000);
    }
    return samples;
}

TEST_CASE("Audio buffer", "[structure]") {
    AudioBuffer audio_buffer;

    REQUIRE_FALSE( audio_buffer.is_initialized() );
    REQUIRE_FALSE( audio_buffer.can_read(1) );
    REQUIRE( audio_buffer.get_nb_samples() == 0 );
    REQUIRE( audio_buffer.get_elapsed_time() == 0.0 );

    SECTION("Initialization constructor") {
        AudioBuffer pre_init(2, 44100);
        REQUIRE(pre_init.is_initialized() );
        REQUIRE_FALSE( pre_init.can_read(1) );
        REQUIRE( pre_init.get_nb_samples() == 0 );
        REQUIRE( pre_init.get_elapsed_time() == 0 );
        REQUIRE( pre_init.get_nb_channels() == 2 );
        REQUIRE( pre_init.get_sample_rate() == 44100 );
    }

    SECTION("Lazy Initialization") {
        audio_buffer.init(2, 44100);
        REQUIRE(audio_buffer.is_initialized() );
        REQUIRE_FALSE( audio_buffer.can_read(1) );
        REQUIRE( audio_buffer.get_nb_samples() == 0 );
        REQUIRE( audio_buffer.get_elapsed_time() == 0 );
        REQUIRE( audio_buffer.get_nb_channels() == 2 );
        REQUIRE( audio_buffer.get_sample_rate() == 44100 );
    }


    SECTION("Reading and Writing") {
        const int nb_channels = 2;
        const int sample_rate = 44100;
        const double expected_buffer_time = 20;

        audio_buffer.init(nb_channels, sample_rate);
        int expected_nb_samples = sample_rate * expected_buffer_time;
        REQUIRE(audio_buffer.get_time() == 0.0);
        float* mock_samples = get_mock_samples(2, expected_nb_samples);
        audio_buffer.write(mock_samples, expected_nb_samples);

        REQUIRE(audio_buffer.get_nb_samples() == expected_nb_samples);
        REQUIRE(audio_buffer.get_time() == 0.0);
        REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
        REQUIRE(audio_buffer.get_end_time() == expected_buffer_time);
        REQUIRE(audio_buffer.can_read());
        REQUIRE(audio_buffer.get_nb_can_read() == expected_nb_samples);
        REQUIRE(audio_buffer.can_read(audio_buffer.get_nb_can_read()));

        SECTION("Multiple Consecutive Writes") { // Audio buffer already has mock_samples written into it
            audio_buffer.write(mock_samples, expected_nb_samples);
            REQUIRE(audio_buffer.get_nb_channels() == nb_channels);
            REQUIRE(audio_buffer.get_sample_rate() == sample_rate);
            REQUIRE(audio_buffer.get_nb_samples() == expected_nb_samples * 2);
            REQUIRE(audio_buffer.get_time() == 0.0);
            REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
            REQUIRE(audio_buffer.get_end_time() == expected_buffer_time * 2);
            REQUIRE(audio_buffer.can_read());
            REQUIRE(audio_buffer.get_nb_can_read() == expected_nb_samples * 2);
        }

        SECTION("Clear and restart") { // Audio buffer already has mock_samples written into it
            audio_buffer.clear_and_restart_at(0);
            audio_buffer.write(mock_samples, expected_nb_samples);
            REQUIRE(audio_buffer.get_nb_channels() == nb_channels);
            REQUIRE(audio_buffer.get_sample_rate() == sample_rate);
            REQUIRE(audio_buffer.get_nb_samples() == expected_nb_samples);
            REQUIRE(audio_buffer.get_time() == 0.0);
            REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
            REQUIRE(audio_buffer.get_end_time() == expected_buffer_time);
            REQUIRE(audio_buffer.can_read());
            REQUIRE(audio_buffer.get_nb_can_read() == expected_nb_samples);
        }

        SECTION("Clear and restart with time") { // Audio buffer already has mock_samples written into it
            const int start_time = 10.0;
            audio_buffer.clear_and_restart_at(start_time);
            audio_buffer.write(mock_samples, expected_nb_samples);
            REQUIRE(audio_buffer.get_nb_channels() == nb_channels);
            REQUIRE(audio_buffer.get_sample_rate() == sample_rate);
            REQUIRE(audio_buffer.get_nb_samples() == expected_nb_samples);
            REQUIRE(audio_buffer.get_time() == start_time);
            REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
            REQUIRE(audio_buffer.get_end_time() == expected_buffer_time + start_time);
            REQUIRE(audio_buffer.can_read());
            REQUIRE(audio_buffer.get_nb_can_read() == expected_nb_samples);
        }

        SECTION("Clear and restart after write with time") { // Audio buffer already has mock_samples written into it
            const int start_time = 10.0;
            audio_buffer.write(mock_samples, expected_nb_samples);

            REQUIRE(audio_buffer.get_nb_channels() == nb_channels);
            REQUIRE(audio_buffer.get_sample_rate() == sample_rate);

            audio_buffer.clear_and_restart_at(start_time);

            REQUIRE(audio_buffer.get_nb_channels() == nb_channels);
            REQUIRE(audio_buffer.get_sample_rate() == sample_rate);

            audio_buffer.write(mock_samples, expected_nb_samples);

            REQUIRE(audio_buffer.get_nb_channels() == nb_channels);
            REQUIRE(audio_buffer.get_sample_rate() == sample_rate);

            REQUIRE(audio_buffer.get_nb_samples() == expected_nb_samples);
            REQUIRE(audio_buffer.get_time() == start_time);
            REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
            REQUIRE(audio_buffer.get_end_time() == expected_buffer_time + start_time);
            REQUIRE(audio_buffer.can_read());
            REQUIRE(audio_buffer.get_nb_can_read() == expected_nb_samples);
        }

        SECTION("Clear") { // Audio buffer already has mock_samples written into it
            audio_buffer.clear();
            REQUIRE(audio_buffer.get_nb_channels() == nb_channels);
            REQUIRE(audio_buffer.get_sample_rate() == sample_rate);

            REQUIRE(audio_buffer.get_nb_samples() == 0);
            REQUIRE(audio_buffer.get_time() == 0);
            REQUIRE(audio_buffer.get_elapsed_time() == 0.0);
            REQUIRE(audio_buffer.get_end_time() == 0);
            REQUIRE_FALSE(audio_buffer.can_read());
            REQUIRE_FALSE(audio_buffer.can_read(1));
            REQUIRE(audio_buffer.get_nb_can_read() == 0);
        }

        SECTION("Set time") { // Audio buffer already has mock_samples written into it
            const double time_to_set = expected_buffer_time / 2;
            audio_buffer.set_time(time_to_set);
            REQUIRE(audio_buffer.get_nb_channels() == nb_channels);
            REQUIRE(audio_buffer.get_sample_rate() == sample_rate);

            REQUIRE(audio_buffer.get_elapsed_time() == time_to_set);
            REQUIRE(audio_buffer.get_end_time() == expected_buffer_time);
            REQUIRE(audio_buffer.get_time() == time_to_set);
            REQUIRE(audio_buffer.get_nb_can_read() == expected_nb_samples / 2);
            REQUIRE(audio_buffer.can_read());
            REQUIRE(audio_buffer.can_read(1));
            REQUIRE(audio_buffer.get_nb_samples() == expected_nb_samples);
        }

        SECTION("Writing one sample") {
            AudioBuffer audio_buffer(nb_channels, sample_rate);
            float sample[2] = {0.0, 1.0};
            audio_buffer.write(sample, 1);
            REQUIRE(audio_buffer.can_read(1));
            REQUIRE(audio_buffer.can_read());
            REQUIRE(audio_buffer.get_nb_can_read() == 1);
            REQUIRE(audio_buffer.can_read(audio_buffer.get_nb_can_read()));
        }

        SECTION("Writing two samples") {
            AudioBuffer audio_buffer(nb_channels, sample_rate);
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


}