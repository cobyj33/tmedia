cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ASCII_VIDEO_VERSION_STRING "0.4.0")
set(ASCII_VIDEO_VERSION_MAJOR 0)
set(ASCII_VIDEO_VERSION_MINOR 4)
set(ASCII_VIDEO_VERSION_PATCH 0)

set(ASCII_VIDEO_SUPPORTED_LANGUAGES CXX C)
if (APPLE)
    list (APPEND ASCII_VIDEO_SUPPORTED_LANGUAGES OBJC OBJCXX)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message("No Build Type Given, setting build type to Release")
endif()
message("Building Build Type: " ${CMAKE_BUILD_TYPE})

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and run cmake from there")
endif()

project(ascii_video
VERSION 0.4.0
LANGUAGES ${ASCII_VIDEO_SUPPORTED_LANGUAGES}
DESCRIPTION "Terminal video media player")

option(ASCII_VIDEO_BUILD_TESTS "Build Testing Executable for ascii_video" OFF)
option(FALLBACK_BUILD_FROM_SHARED "Will build dependencies if the shared version is requested but is not found" ON)
option(USE_SHARED_NCURSES "Use globally detected NCurses build" OFF)
option(USE_SHARED_FFMPEG "Use globally detected FFmpeg" OFF)
option(USE_SHARED_ALL "Use All Globally detected libraries" OFF)

option(BUILD_FFMPEG_AS_SHARED "Build FFmpeg Library as a shared library. " OFF)
option(BUILD_NCURSES_AS_SHARED "Build NCurses as a shared library." OFF)

if (ASCII_VIDEO_BUILD_TESTS)
    message("Configured to build testing executable")
else()
    message("Configured to NOT build testing executable")
endif()

if(NOT USE_SHARED_ALL)
    add_subdirectory(${CMAKE_SOURCE_DIR}/lib)
else()
    find_package(Curses REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(
        LIBAV REQUIRED IMPORTED_TARGET
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
    )
endif()

if(ASCII_VIDEO_BUILD_TESTS)
    add_subdirectory(./lib/Catch2)
endif()

add_library(miniaudio STATIC ./lib/miniaudio/miniaudio.c)
target_include_directories(miniaudio PRIVATE ./lib/miniaudio)
    
set(COMMON_SOURCE_FILES 
./src/audioresampler.cpp
./src/videoconverter.cpp
./src/streamdata.cpp
./src/ascii.cpp
./src/audiobuffer.cpp
./src/audio.cpp
./src/audio_thread.cpp
./src/decode.cpp
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
./src/scale.cpp
./src/ui/gui.cpp
./src/termcolor.cpp
)

message("Curses Libraries found at " ${CURSES_LIBRARIES})

add_executable(ascii_video ${COMMON_SOURCE_FILES} ./src/main.cpp)
target_compile_features(ascii_video PRIVATE cxx_std_17)
target_include_directories(ascii_video PRIVATE ${CURSES_INCLUDE_DIR} ./include ./lib/miniaudio ./lib/argparse )
target_link_libraries(ascii_video PRIVATE PkgConfig::LIBAV ${CURSES_LIBRARIES} miniaudio)
add_dependencies(ascii_video PkgConfig::LIBAV)

if(MSVC)
    target_compile_options(ascii_video PRIVATE /std:c++17 /W4 /WX)
else()
    target_compile_options(ascii_video PRIVATE -Wall -Wextra -Wpedantic) # Consider adding -Werror back
endif()

set(TEST_SOURCE_FILES
./src/tests/test_audiobuffer.cpp
./src/tests/test_playback.cpp
./src/tests/test_wmath.cpp
./src/tests/test_scale.cpp
./src/tests/test_pixeldata.cpp
./src/tests/test_color.cpp
./src/tests/test_formatting.cpp
)

if (ASCII_VIDEO_BUILD_TESTS)
    add_executable(ascii_tests ${COMMON_SOURCE_FILES} ${TEST_SOURCE_FILES})
    if(MSVC)
        target_compile_options(ascii_tests PRIVATE /std:c++17 /W4 /WX)
    else()
        target_compile_options(ascii_tests PRIVATE -Wall -Wextra -Wpedantic)# Consider adding -Werror back
    endif()
    target_include_directories(ascii_tests PRIVATE ${CURSES_INCLUDE_DIR} ./include ./lib/miniaudio ./lib/argparse )
    target_compile_features(ascii_tests PRIVATE cxx_std_17)
    target_link_libraries(ascii_tests PRIVATE Catch2::Catch2WithMain ${CURSES_LIBRARIES} PkgConfig::LIBAV miniaudio)
endif()
