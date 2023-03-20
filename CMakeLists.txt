cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message("No Build Type Given, setting build type to Release")
endif()

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and run cmake from there")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

option(ASCII_VIDEO_BUILD_TESTS "Build Testing Executable for ascii_video" OFF)

message("${CMAKE_SYSTEM_PROGRAM_PATH}")

if (ASCII_VIDEO_BUILD_TESTS)
    message("Configured to build testing executable")
else()
    message("Configured to NOT build testing executable")
endif()

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")


project(ascii_video
VERSION 0.4.0
LANGUAGES CXX C
DESCRIPTION "Terminal video media player")

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

if (ASCII_VIDEO_BUILD_TESTS)
    add_subdirectory(./lib/Catch2)
endif()

include_directories( ${CURSES_INCLUDE_DIR} ./includes ./lib/miniaudio/extras/miniaudio_split ./lib/argparse/include/argparse )

set(COMMON_SOURCE_FILES 
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
./src/threads.cpp
./src/video_thread.cpp
./src/formatting.cpp
./src/wmath.cpp
./src/renderer.cpp
./src/wtime.cpp
./src/color.cpp
./src/icons.cpp
./src/playback.cpp
./src/pixeldata.cpp
./src/aabb.cpp
./src/scale.cpp
./src/ui/gui.cpp
./src/termcolor.cpp
)

set(TEST_SOURCE_FILES
./src/tests/test_audiostream.cpp
./src/tests/test_playback.cpp
./src/tests/test_wmath.cpp
./src/tests/test_scale.cpp
./src/tests/test_pixeldata.cpp
./src/tests/test_color.cpp
./src/tests/test_formatting.cpp
)

add_executable(ascii_video ${COMMON_SOURCE_FILES} ./src/main.cpp)
target_link_libraries(ascii_video PkgConfig::LIBAV ${CURSES_LIBRARIES})

if (ASCII_VIDEO_BUILD_TESTS)
    add_executable(tests ${COMMON_SOURCE_FILES} ${TEST_SOURCE_FILES})
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain ${CURSES_LIBRARIES} PkgConfig::LIBAV)
endif()