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
list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib )

# Note that it seems like CMake appends CMAKE_PREFIX_PATH onto the searched
# pkg-config directories, instead of prepending them. If you want to say where
# pkg-config should search first, I'd still recommend setting the PKG_CONFIG_PATH
# environment variable in your shell.
# set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON) 

set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
list(APPEND CMAKE_INSTALL_RPATH "$\{ORIGIN\}")
list(APPEND CMAKE_INSTALL_RPATH "$\{ORIGIN\}/lib")

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
# set(TMEDIA_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Wl,-rpath,'$ORIGIN'/lib)
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
option(FIND_FFMPEG "Find FFmpeg Libraries on system rather than build FFmpeg alongside tmedia. Default ON" ON)
option(FIND_CURSES "Find Curses Libraries on system rather than build NCurses alongside tmedia. Default ON" ON)
option(FIND_FMT "Find fmt library on system rather than build fmt alongside tmedia. Default OFF" OFF)
option(FIND_CATCH2 "Find Catch2 library on system rather than build Catch2 alongside tmedia ** when configured for testing with TMEDIA_BUILD_TESTS=ON **. Default OFF" OFF)

message("+---TMEDIA-BUILD-CONFIGURATION---------------------------------------")
message("| Options: CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})
message("| Options: TMEDIA_BUILD_TESTS: " ${TMEDIA_BUILD_TESTS})

message("| Options: FIND_FFMPEG: " ${FIND_FFMPEG})
if (NOT FIND_FFMPEG)
message("| Options: FFMPEG_EXTRA_CONFIGURE_OPTIONS: " ${FFMPEG_EXTRA_CONFIGURE_OPTIONS})
endif()

message("| Options: FIND_CURSES: " ${FIND_CURSES})
if(NOT FIND_CURSES)
message("| Options: NCURSES_EXTRA_CONFIGURE_OPTIONS: " ${NCURSES_EXTRA_CONFIGURE_OPTIONS})
endif()

message("| Options: FIND_FMT: " ${FIND_FMT})
message("| Options: FIND_CATCH2: " ${FIND_CATCH2})
message("+--------------------------------------------------------------------")

# Keep COMMON_SOURCE_FILES list in alphabetical order separated by
# module
set(COMMON_SOURCE_FILES 
${CMAKE_SOURCE_DIR}/src/audio/audio_visualizer.cpp
${CMAKE_SOURCE_DIR}/src/audio/audio.cpp
${CMAKE_SOURCE_DIR}/src/audio/audioringbuffer.cpp
${CMAKE_SOURCE_DIR}/src/audio/blocking_audioringbuffer.cpp
${CMAKE_SOURCE_DIR}/src/audio/maaudioout.cpp
${CMAKE_SOURCE_DIR}/src/audio/wminiaudio.cpp

${CMAKE_SOURCE_DIR}/src/cmdlpar/cmdlpar.cpp

${CMAKE_SOURCE_DIR}/src/ffmpeg/audioresampler.cpp
${CMAKE_SOURCE_DIR}/src/ffmpeg/boiler.cpp
${CMAKE_SOURCE_DIR}/src/ffmpeg/decode.cpp
${CMAKE_SOURCE_DIR}/src/ffmpeg/deleter.cpp
${CMAKE_SOURCE_DIR}/src/ffmpeg/ffmpeg_error.cpp
${CMAKE_SOURCE_DIR}/src/ffmpeg/probe.cpp
${CMAKE_SOURCE_DIR}/src/ffmpeg/streamdecoder.cpp
${CMAKE_SOURCE_DIR}/src/ffmpeg/videoconverter.cpp

${CMAKE_SOURCE_DIR}/src/image/ascii.cpp
${CMAKE_SOURCE_DIR}/src/image/color.cpp
${CMAKE_SOURCE_DIR}/src/image/palette.cpp
${CMAKE_SOURCE_DIR}/src/image/palette_io.cpp
${CMAKE_SOURCE_DIR}/src/image/pixeldata.cpp

${CMAKE_SOURCE_DIR}/src/media/audio_thread.cpp
${CMAKE_SOURCE_DIR}/src/media/duration_checking.cpp
${CMAKE_SOURCE_DIR}/src/media/mediaclock.cpp
${CMAKE_SOURCE_DIR}/src/media/mediadecoder.cpp
${CMAKE_SOURCE_DIR}/src/media/mediafetcher.cpp
${CMAKE_SOURCE_DIR}/src/media/mediaformat.cpp
${CMAKE_SOURCE_DIR}/src/media/metadata.cpp
${CMAKE_SOURCE_DIR}/src/media/playlist.cpp
${CMAKE_SOURCE_DIR}/src/media/video_thread.cpp

${CMAKE_SOURCE_DIR}/src/tmcurses/tmcurses_print.cpp
${CMAKE_SOURCE_DIR}/src/tmcurses/tmcurses_init.cpp
${CMAKE_SOURCE_DIR}/src/tmcurses/tmcurses.cpp

${CMAKE_SOURCE_DIR}/src/util/formatting.cpp
${CMAKE_SOURCE_DIR}/src/util/sleep.cpp


${CMAKE_SOURCE_DIR}/src/tmedia_cli.cpp
${CMAKE_SOURCE_DIR}/src/tmedia_tui.cpp
${CMAKE_SOURCE_DIR}/src/tmedia_tui_elems.cpp
)

set(TEST_SOURCE_FILES
${CMAKE_SOURCE_DIR}/src/tests/test_color.cpp
${CMAKE_SOURCE_DIR}/src/tests/test_cmdlpar.cpp
${CMAKE_SOURCE_DIR}/src/tests/test_formatting.cpp
${CMAKE_SOURCE_DIR}/src/tests/test_mediaclock.cpp
${CMAKE_SOURCE_DIR}/src/tests/test_scale.cpp
${CMAKE_SOURCE_DIR}/src/tests/test_unitconvert.cpp
${CMAKE_SOURCE_DIR}/src/tests/test_palette_io_gpl.cpp
${CMAKE_SOURCE_DIR}/src/tests/test_wmath.cpp
${CMAKE_SOURCE_DIR}/src/tests/test_playlist.cpp
)

project(tmedia
VERSION ${TMEDIA_VERSION}
LANGUAGES ${TMEDIA_SUPPORTED_LANGUAGES}
DESCRIPTION "Terminal video media player")

configure_file(${CMAKE_SOURCE_DIR}/include/tmedia/version.h.in ${CMAKE_BINARY_DIR}/include/tmedia/version.h @ONLY)
add_executable(tmedia ${COMMON_SOURCE_FILES} ${CMAKE_SOURCE_DIR}/src/main.cpp ${CMAKE_SOURCE_DIR}/src/tmedia.cpp)
include(${CMAKE_SOURCE_DIR}/lib/deps.cmake)

message("+---TMEDIA-DEPENDENCY-CONFIG---------------------------------------")
message("Third Party Libraries:")
printlist("${TMEDIA_DEPS_LIBRARIES}\n")
message("Third Party Includes:")
printlist("${TMEDIA_DEPS_INCLUDE_DIRS}\n")
message("Target Dependencies:")
printlist("${TMEDIA_TARGET_DEPENDENCIES}\n")
message("+------------------------------------------------------------------")

target_compile_features(tmedia PRIVATE cxx_std_17)
target_include_directories(tmedia PRIVATE ${TMEDIA_DEPS_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/include )
target_link_libraries(tmedia PRIVATE ${TMEDIA_DEPS_LIBRARIES} ${CMAKE_DL_LIBS})
target_compile_options(tmedia PRIVATE ${TMEDIA_COMPILE_OPTIONS})
if (NOT TMEDIA_TARGET_DEPENDENCIES STREQUAL "")
  add_dependencies(tmedia ${TMEDIA_TARGET_DEPENDENCIES})
endif()


if (TMEDIA_BUILD_TESTS)
  message("Configured to build testing executable")
  
  if (FIND_CATCH2)
    find_package(Catch2 3)
    if (NOT TARGET Catch2::Catch2WithMain)
      message(FATAL_ERROR "FIND_CATCH2 enabled but Catch2 v3 library could not be found on the system.")
    endif()
  else()
    message("Building Catch2 v3.0.1 with tests")
    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG    v3.0.1 # or a later release
    )
    FetchContent_MakeAvailable(Catch2)
  endif()

  add_executable(tmedia_tests ${COMMON_SOURCE_FILES} ${TEST_SOURCE_FILES})
  target_compile_options(tmedia_tests PRIVATE ${TMEDIA_COMPILE_OPTIONS})
  target_include_directories(tmedia_tests PRIVATE ${TMEDIA_DEPS_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/include)
  target_compile_features(tmedia_tests PRIVATE cxx_std_17)
  target_link_libraries(tmedia_tests PRIVATE Catch2::Catch2WithMain ${TMEDIA_DEPS_LIBRARIES} ${CMAKE_DL_LIBS})

  if (NOT TMEDIA_TARGET_DEPENDENCIES STREQUAL "")
    add_dependencies(tmedia_tests ${TMEDIA_TARGET_DEPENDENCIES})
  endif()
endif()
