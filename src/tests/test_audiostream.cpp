#include "audiostream.h"
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

TEST_CASE("Audio Stream", "[structure]") {
    AudioStream audio_stream;

    REQUIRE_FALSE( audio_stream.is_initialized() );
    REQUIRE_FALSE( audio_stream.can_read(1) );
    REQUIRE( audio_stream.get_nb_samples() == 0 );
    REQUIRE( audio_stream.get_elapsed_time() == 0.0 );

    SECTION("Initialization constructor") {
        AudioStream pre_init(2, 44100);
        REQUIRE(pre_init.is_initialized() );
        REQUIRE_FALSE( pre_init.can_read(1) );
        REQUIRE( pre_init.get_nb_samples() == 0 );
        REQUIRE( pre_init.get_elapsed_time() == 0 );
        REQUIRE( pre_init.get_nb_channels() == 2 );
        REQUIRE( pre_init.get_sample_rate() == 44100 );
    }

    SECTION("Lazy Initialization") {
        audio_stream.init(2, 44100);
        REQUIRE(audio_stream.is_initialized() );
        REQUIRE_FALSE( audio_stream.can_read(1) );
        REQUIRE( audio_stream.get_nb_samples() == 0 );
        REQUIRE( audio_stream.get_elapsed_time() == 0 );
        REQUIRE( audio_stream.get_nb_channels() == 2 );
        REQUIRE( audio_stream.get_sample_rate() == 44100 );
    }


    SECTION("Reading and Writing") {
        const int nb_channels = 2;
        const int sample_rate = 44100;
        const double expected_stream_time = 20;

        audio_stream.init(nb_channels, sample_rate);
        int expected_nb_samples = sample_rate * expected_stream_time;
        REQUIRE(audio_stream.get_time() == 0.0);
        float* mock_samples = get_mock_samples(2, expected_nb_samples);
        audio_stream.write(mock_samples, expected_nb_samples);

        REQUIRE(audio_stream.get_nb_samples() == expected_nb_samples);
        REQUIRE(audio_stream.get_time() == 0.0);
        REQUIRE(audio_stream.get_elapsed_time() == 0.0);
        REQUIRE(audio_stream.get_end_time() == expected_stream_time);
        REQUIRE(audio_stream.can_read());
        REQUIRE(audio_stream.get_nb_can_read() == expected_nb_samples);
        REQUIRE(audio_stream.can_read(audio_stream.get_nb_can_read()));

        SECTION("Multiple Consecutive Writes") { // Audio stream already has mock_samples written into it
            audio_stream.write(mock_samples, expected_nb_samples);
            REQUIRE(audio_stream.get_nb_channels() == nb_channels);
            REQUIRE(audio_stream.get_sample_rate() == sample_rate);
            REQUIRE(audio_stream.get_nb_samples() == expected_nb_samples * 2);
            REQUIRE(audio_stream.get_time() == 0.0);
            REQUIRE(audio_stream.get_elapsed_time() == 0.0);
            REQUIRE(audio_stream.get_end_time() == expected_stream_time * 2);
            REQUIRE(audio_stream.can_read());
            REQUIRE(audio_stream.get_nb_can_read() == expected_nb_samples * 2);
        }

        SECTION("Clear and restart") { // Audio stream already has mock_samples written into it
            audio_stream.clear_and_restart_at(0);
            audio_stream.write(mock_samples, expected_nb_samples);
            REQUIRE(audio_stream.get_nb_channels() == nb_channels);
            REQUIRE(audio_stream.get_sample_rate() == sample_rate);
            REQUIRE(audio_stream.get_nb_samples() == expected_nb_samples);
            REQUIRE(audio_stream.get_time() == 0.0);
            REQUIRE(audio_stream.get_elapsed_time() == 0.0);
            REQUIRE(audio_stream.get_end_time() == expected_stream_time);
            REQUIRE(audio_stream.can_read());
            REQUIRE(audio_stream.get_nb_can_read() == expected_nb_samples);
        }

        SECTION("Clear and restart with time") { // Audio stream already has mock_samples written into it
            const int start_time = 10.0;
            audio_stream.clear_and_restart_at(start_time);
            audio_stream.write(mock_samples, expected_nb_samples);
            REQUIRE(audio_stream.get_nb_channels() == nb_channels);
            REQUIRE(audio_stream.get_sample_rate() == sample_rate);
            REQUIRE(audio_stream.get_nb_samples() == expected_nb_samples);
            REQUIRE(audio_stream.get_time() == start_time);
            REQUIRE(audio_stream.get_elapsed_time() == 0.0);
            REQUIRE(audio_stream.get_end_time() == expected_stream_time + start_time);
            REQUIRE(audio_stream.can_read());
            REQUIRE(audio_stream.get_nb_can_read() == expected_nb_samples);
        }

        SECTION("Clear and restart after write with time") { // Audio stream already has mock_samples written into it
            const int start_time = 10.0;
            audio_stream.write(mock_samples, expected_nb_samples);

            REQUIRE(audio_stream.get_nb_channels() == nb_channels);
            REQUIRE(audio_stream.get_sample_rate() == sample_rate);

            audio_stream.clear_and_restart_at(start_time);

            REQUIRE(audio_stream.get_nb_channels() == nb_channels);
            REQUIRE(audio_stream.get_sample_rate() == sample_rate);

            audio_stream.write(mock_samples, expected_nb_samples);

            REQUIRE(audio_stream.get_nb_channels() == nb_channels);
            REQUIRE(audio_stream.get_sample_rate() == sample_rate);

            REQUIRE(audio_stream.get_nb_samples() == expected_nb_samples);
            REQUIRE(audio_stream.get_time() == start_time);
            REQUIRE(audio_stream.get_elapsed_time() == 0.0);
            REQUIRE(audio_stream.get_end_time() == expected_stream_time + start_time);
            REQUIRE(audio_stream.can_read());
            REQUIRE(audio_stream.get_nb_can_read() == expected_nb_samples);
        }

        SECTION("Clear") { // Audio stream already has mock_samples written into it
            audio_stream.clear();
            REQUIRE(audio_stream.get_nb_channels() == nb_channels);
            REQUIRE(audio_stream.get_sample_rate() == sample_rate);

            REQUIRE(audio_stream.get_nb_samples() == 0);
            REQUIRE(audio_stream.get_time() == 0);
            REQUIRE(audio_stream.get_elapsed_time() == 0.0);
            REQUIRE(audio_stream.get_end_time() == 0);
            REQUIRE_FALSE(audio_stream.can_read());
            REQUIRE_FALSE(audio_stream.can_read(1));
            REQUIRE(audio_stream.get_nb_can_read() == 0);
        }

        SECTION("Set time") { // Audio stream already has mock_samples written into it
            const double time_to_set = expected_stream_time / 2;
            audio_stream.set_time(time_to_set);
            REQUIRE(audio_stream.get_nb_channels() == nb_channels);
            REQUIRE(audio_stream.get_sample_rate() == sample_rate);

            REQUIRE(audio_stream.get_elapsed_time() == time_to_set);
            REQUIRE(audio_stream.get_end_time() == expected_stream_time);
            REQUIRE(audio_stream.get_time() == time_to_set);
            REQUIRE(audio_stream.get_nb_can_read() == expected_nb_samples / 2);
            REQUIRE(audio_stream.can_read());
            REQUIRE(audio_stream.can_read(1));
            REQUIRE(audio_stream.get_nb_samples() == expected_nb_samples);
        }

        SECTION("Writing one sample") {
            AudioStream audio_stream(nb_channels, sample_rate);
            float sample[2] = {0.0, 1.0};
            audio_stream.write(sample, 1);
            REQUIRE(audio_stream.can_read(1));
            REQUIRE(audio_stream.can_read());
            REQUIRE(audio_stream.get_nb_can_read() == 1);
            REQUIRE(audio_stream.can_read(audio_stream.get_nb_can_read()));
        }

        SECTION("Writing two samples") {
            AudioStream audio_stream(nb_channels, sample_rate);
            float sample[4] = {0.0, 1.0, 1.0, 0.0};
            audio_stream.write(sample, 2);
            REQUIRE(audio_stream.can_read(2));
            REQUIRE(audio_stream.can_read(1));
            REQUIRE(audio_stream.can_read());
            REQUIRE(audio_stream.get_nb_can_read() == 2);
            REQUIRE(audio_stream.can_read(audio_stream.get_nb_can_read()));
        }

        std::free(mock_samples);
    } // "Reading and Writing"


}