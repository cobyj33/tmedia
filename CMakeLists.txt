cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED 14)

project(ascii_video)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

find_package(Curses REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

add_subdirectory(lib/Catch2)

include_directories( ${CURSES_INCLUDE_DIR} ./includes ./lib/miniaudio/extras/miniaudio_split ./lib/argparse/include/argparse )

add_executable(ascii_video
./lib/miniaudio/extras/miniaudio_split/miniaudio.c
./src/audioresampler.cpp
./src/videoconverter.cpp
./src/streamdata.cpp
./src/main.cpp
./src/ascii.cpp
./src/audiostream.cpp
./src/audio.cpp
./src/audio_thread.cpp
./src/decode.cpp
./src/image.cpp
./src/media.cpp
./src/boiler.cpp
./src/init.cpp
./src/info.cpp
./src/threads.cpp
./src/video.cpp
./src/formatting.cpp
./src/wmath.cpp
./src/renderer.cpp
./src/loader.cpp
./src/wtime.cpp
./src/debug.cpp
./src/color.cpp
./src/icons.cpp
./src/playback.cpp
./src/curses_helpers.cpp
./src/pixeldata.cpp
./src/aabb.cpp
./src/scale.cpp
./src/termcolor.cpp
)

target_link_libraries( ascii_video ${CURSES_LIBRARIES} PkgConfig::LIBAV)

add_executable(tests 
./lib/miniaudio/extras/miniaudio_split/miniaudio.c
./src/audioresampler.cpp
./src/videoconverter.cpp
./src/streamdata.cpp
./src/ascii.cpp
./src/audiostream.cpp
./src/audio.cpp
./src/audio_thread.cpp
./src/decode.cpp
./src/image.cpp
./src/media.cpp
./src/boiler.cpp
./src/init.cpp
./src/info.cpp
./src/threads.cpp
./src/video.cpp
./src/formatting.cpp
./src/wmath.cpp
./src/renderer.cpp
./src/loader.cpp
./src/wtime.cpp
./src/debug.cpp
./src/color.cpp
./src/icons.cpp
./src/playback.cpp
./src/curses_helpers.cpp
./src/pixeldata.cpp
./src/aabb.cpp
./src/scale.cpp
./src/termcolor.cpp

./src/tests/test_audiostream.cpp
./src/tests/test_playback.cpp
./src/tests/test_wmath.cpp
./src/tests/test_scale.cpp
./src/tests/test_playheadlist.cpp
./src/tests/test_pixeldata.cpp
)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain ${CURSES_LIBRARIES} PkgConfig::LIBAV)