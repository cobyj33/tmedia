cmake_minimum_required(VERSION 3.10)

function(printlist list) # This has to be called with the list in "${a_string_like_this}"
  foreach(line IN LISTS list)
    message("${line}")
  endforeach()
endfunction()

include(FetchContent)
include(ExternalProject)

if (WIN32)
  message(WARNING "Windows builds are completely untested as of now. Good luck")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#Used in configuring version file
set(TMEDIA_VERSION_MAJOR 0)
set(TMEDIA_VERSION_MINOR 5)
set(TMEDIA_VERSION_PATCH 0)
set(TMEDIA_VERSION "${TMEDIA_VERSION_MAJOR}.${TMEDIA_VERSION_MINOR}.${TMEDIA_VERSION_PATCH}")

set(TMEDIA_DEPS_DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/deps)
set(TMEDIA_DEPS_INSTALL_DIR ${CMAKE_BINARY_DIR})
set(TMEDIA_DEPS_LIBRARIES "")
set(TMEDIA_DEPS_INCLUDE_DIRS "")
set(TMEDIA_TARGET_DEPENDENCIES "")

set(TMEDIA_COMPILE_OPTIONS -Wall -Wextra -Wpedantic)
if (MSVC)
  set(TMEDIA_COMPILE_OPTIONS /std:c++17 /W4 /WX)
endif()

set(TMEDIA_SUPPORTED_LANGUAGES CXX C)
if (APPLE)
  list (APPEND TMEDIA_SUPPORTED_LANGUAGES OBJC OBJCXX)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message("No Build Type Given, setting build type to Release")
endif()
message("Building Build Type: " ${CMAKE_BUILD_TYPE})

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "Building in-source is not supported! Create a build dir and run cmake from there")
endif()

option(TMEDIA_BUILD_TESTS "Build Testing Executable for tmedia" OFF)
option(FIND_FFMPEG "Find FFmpeg Libraries on system rather than build FFmpeg alongside tmedia" ON)
option(FIND_CURSES "Find Curses Libraries on system rather than build NCurses alongside tmedia" ON)

set(TMEDIA_DEP_JOBS "4" CACHE STRING "Set the number of jobs used when building dependencies")

message("Options: TMEDIA_BUILD_TESTS: " ${TMEDIA_BUILD_TESTS})
message("Options: FIND_FFMPEG: " ${FIND_FFMPEG})
message("Options: FIND_CURSES: " ${FIND_CURSES})
message("Options: TMEDIA_DEP_JOBS: " ${TMEDIA_DEP_JOBS})
message("Options: NCURSES_EXTRA_CONFIGURE_FLAGS: " ${NCURSES_EXTRA_CONFIGURE_FLAGS})
message("Options: FFMPEG_EXTRA_CONFIGURE_FLAGS: " ${FFMPEG_EXTRA_CONFIGURE_FLAGS})


set(COMMON_SOURCE_FILES 
./src/ascii.cpp
./src/audio_thread.cpp
./src/audio_visualizer.cpp
./src/audio.cpp
./src/maaudioout.cpp
./src/audioresampler.cpp
./src/audioringbuffer.cpp
./src/blocking_audioringbuffer.cpp
./src/boiler.cpp
./src/canvas.cpp
./src/color.cpp
./src/cli_iter.cpp
./src/tmedia_cli.cpp
./src/decode.cpp
./src/duration_checking.cpp
./src/ffmpeg_error.cpp
./src/formatting.cpp
./src/mediaclock.cpp
./src/mediadecoder.cpp
./src/mediafetcher.cpp
./src/mediaformat.cpp
./src/metadata.cpp
./src/palette.cpp
./src/palette_io.cpp
./src/pixeldata.cpp
./src/playlist.cpp
./src/probe.cpp
./src/scale.cpp
./src/sleep.cpp
./src/streamdecoder.cpp
./src/tmcurses_print.cpp
./src/tmcurses.cpp
./src/tmedia_tui.cpp
./src/tmedia_tui_elems.cpp
./src/video_thread.cpp
./src/videoconverter.cpp
./src/wmath.cpp
./src/wminiaudio.cpp
./src/wtime.cpp)

set(TEST_SOURCE_FILES
./src/tests/test_color.cpp
./src/tests/test_cli_iter.cpp
./src/tests/test_formatting.cpp
./src/tests/test_mediaclock.cpp
./src/tests/test_pixeldata.cpp
./src/tests/test_scale.cpp
./src/tests/test_unitconvert.cpp
./src/tests/test_palette_io_gpl.cpp
./src/tests/test_wmath.cpp
)

set(NCURSES_CONFIGURE_OPTIONS --prefix=${CMAKE_BINARY_DIR}
--datadir=${CMAKE_BINARY_DIR}/data
--without-tests
--without-manpages
--without-progs
--without-debug
--with-shared
--without-normal)

set(FFMPEG_BUILD_OPTIONS --prefix=${CMAKE_BINARY_DIR}
--pkg-config-flags=--static
--extra-cflags=-I${CMAKE_BINARY_DIR}/include
--extra-ldflags=-L${CMAKE_BINARY_DIR}/lib
--extra-libs=-lpthread
--extra-libs=-lm
--enable-rpath
--enable-shared
--disable-static
--disable-network
--disable-encoders
--disable-muxers
--disable-programs
--disable-doc
--disable-filters
--disable-postproc
--disable-avfilter
--disable-avdevice
--disable-protocols
--disable-devices
--enable-nonfree
--enable-protocol=file,pipe)
# --disable-autodetect might be wanted for builds for other systems as well
# Note that this is automatically set to be nonfree license

project(tmedia
VERSION ${TMEDIA_VERSION}
LANGUAGES ${TMEDIA_SUPPORTED_LANGUAGES}
DESCRIPTION "Terminal video media player")

add_executable(tmedia ${COMMON_SOURCE_FILES} ./src/main.cpp ./src/tmedia.cpp)
configure_file(${CMAKE_SOURCE_DIR}/include/version.h.in ${CMAKE_BINARY_DIR}/include/version.h @ONLY)
include(./lib/deps.cmake)

message("\nThird Party Libs:")
printlist("${TMEDIA_DEPS_LIBRARIES}")
message("\nThird Party Includes:")
printlist("${TMEDIA_DEPS_INCLUDE_DIRS}")
message("\nTarget Dependencies:")
printlist("${TMEDIA_TARGET_DEPENDENCIES}")

target_compile_features(tmedia PRIVATE cxx_std_17)
target_include_directories(tmedia PRIVATE ${TMEDIA_DEPS_INCLUDE_DIRS} ./include ${CMAKE_CURRENT_BINARY_DIR}/include )
target_link_libraries(tmedia PRIVATE ${TMEDIA_DEPS_LIBRARIES} miniaudio NaturalSort readerwriterqueue random ${CMAKE_DL_LIBS})
target_compile_options(tmedia PRIVATE ${TMEDIA_COMPILE_OPTIONS})
if (NOT TMEDIA_TARGET_DEPENDENCIES STREQUAL "")
  add_dependencies(tmedia ${TMEDIA_TARGET_DEPENDENCIES})
endif()


if (TMEDIA_BUILD_TESTS)
  message("Configured to build testing executable")
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG    v3.0.1 # or a later release
  )

  FetchContent_MakeAvailable(Catch2)

  add_executable(tmedia_tests ${COMMON_SOURCE_FILES} ${TEST_SOURCE_FILES})
  target_compile_options(tmedia_tests PRIVATE ${TMEDIA_COMPILE_OPTIONS})
  target_include_directories(tmedia_tests PRIVATE ${TMEDIA_DEPS_INCLUDE_DIRS} ./include ${CMAKE_CURRENT_BINARY_DIR}/include)
  target_compile_features(tmedia_tests PRIVATE cxx_std_17)
  target_link_libraries(tmedia_tests PRIVATE Catch2::Catch2WithMain ${TMEDIA_DEPS_LIBRARIES} miniaudio NaturalSort readerwriterqueue random ${CMAKE_DL_LIBS})

  if (NOT TMEDIA_TARGET_DEPENDENCIES STREQUAL "")
    add_dependencies(tmedia_tests ${TMEDIA_TARGET_DEPENDENCIES})
  endif()
else()
  message("Configured to NOT build testing executable")
endif()
