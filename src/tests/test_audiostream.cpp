#include "audiostream.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Audio Stream", "[structure]") {
    AudioStream audio_stream;

    REQUIRE_FALSE( audio_stream.is_initialized() );
    REQUIRE_FALSE( audio_stream.can_read(1) );
    REQUIRE( audio_stream.get_nb_samples() == 0 );
    REQUIRE( audio_stream.get_elapsed_time() == 0.0 );
}