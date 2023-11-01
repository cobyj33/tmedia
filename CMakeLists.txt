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

set(LOCAL_NCURSES_LIB ${CMAKE_SOURCE_DIR}/lib/ncurses)
set(LOCAL_FFMPEG_LIB ${CMAKE_SOURCE_DIR}/lib/ffmpeg)

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

message("Options: TMEDIA_BUILD_TESTS: " ${TMEDIA_BUILD_TESTS})
message("Options: FIND_FFMPEG: " ${FIND_FFMPEG})
message("Options: FIND_CURSES: " ${FIND_CURSES})
message("Options: NCURSES_EXTRA_CONFIGURE_FLAGS: " ${NCURSES_EXTRA_CONFIGURE_FLAGS})
message("Options: FFMPEG_EXTRA_CONFIGURE_FLAGS: " ${FFMPEG_EXTRA_CONFIGURE_FLAGS})

set(TARGET_DEPENDENCIES "")

set(COMMON_SOURCE_FILES 
./src/ascii.cpp
./src/audio_image.cpp
./src/audio_thread.cpp
./src/audio.cpp
./src/audioresampler.cpp
./src/audioringbuffer.cpp
./src/blocking_audioringbuffer.cpp
./src/boiler.cpp
./src/canvas.cpp
./src/color.cpp
./src/decode.cpp
./src/duration_checking.cpp
./src/ffmpeg_error.cpp
./src/formatting.cpp
./src/mediaclock.cpp
./src/mediadecoder.cpp
./src/mediafetcher.cpp
./src/mediatype.cpp
./src/pixeldata.cpp
./src/playlist.cpp
./src/scale.cpp
./src/sleep.cpp
./src/streamdecoder.cpp
./src/tmcurses_print.cpp
./src/tmcurses.cpp
./src/tmedia_render.cpp
./src/tmedia_string.cpp
./src/video_thread.cpp
./src/videoconverter.cpp
./src/wmath.cpp
./src/wminiaudio.cpp
./src/wtime.cpp)

set(TEST_SOURCE_FILES
./src/tests/test_audiobuffer.cpp
./src/tests/test_color.cpp
./src/tests/test_formatting.cpp
./src/tests/test_mediaclock.cpp
./src/tests/test_pixeldata.cpp
./src/tests/test_scale.cpp
./src/tests/test_unitconvert.cpp
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


if(FIND_CURSES)
  find_package(Curses REQUIRED)
elseif(NOT FIND_CURSES AND (UNIX OR APPLE OR MINGW OR CYGWIN))

  if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU"))
    list(APPEND NCURSES_CONFIGURE_OPTIONS --with-build-cflags=-g)
  endif()

  list(APPEND NCURSES_CONFIGURE_OPTIONS ${NCURSES_EXTRA_CONFIGURE_FLAGS})

  message("\nNCurses configure options:")
  printlist("${NCURSES_CONFIGURE_OPTIONS}")

  ExternalProject_Add(
    ncurses6
    SOURCE_DIR    ${LOCAL_NCURSES_LIB}/src
    BINARY_DIR    ${LOCAL_NCURSES_LIB}/build
    PREFIX      ${CMAKE_BINARY_DIR}
    INSTALL_DIR     ${CMAKE_BINARY_DIR}
    CONFIGURE_COMMAND ${LOCAL_NCURSES_LIB}/src/configure ${NCURSES_CONFIGURE_OPTIONS}
    BUILD_COMMAND   make
    INSTALL_COMMAND   make install
    BUILD_IN_SOURCE   OFF
    URL         https://ftp.gnu.org/pub/gnu/ncurses/ncurses-6.4.tar.gz
    TLS_VERIFY    ON
  )

  list(APPEND TARGET_DEPENDENCIES ncurses6)

  set(CURSES_LIBRARIES ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}ncurses${CMAKE_SHARED_LIBRARY_SUFFIX}
  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}form${CMAKE_SHARED_LIBRARY_SUFFIX})
  set(CURSES_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include/ncurses)
else()
  message(WARNING "Currently, Non-Unix builds are completely untested. Good luck.")
endif()

if(FIND_FFMPEG)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET libavcodec
  libavformat
  libavutil
  libswresample
  libswscale)

  target_link_libraries(tmedia PRIVATE PkgConfig::LIBAV)
else()
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU"))
    list(APPEND FFMPEG_BUILD_OPTIONS --extra-cflags=-g)
  endif()

  list(APPEND FFMPEG_BUILD_OPTIONS ${FFMPEG_EXTRA_CONFIGURE_FLAGS})
  message("\nFFMPEG Build Options:")
  printlist("${FFMPEG_BUILD_OPTIONS}")

  ExternalProject_Add(
    FFmpeg
    SOURCE_DIR        ${LOCAL_FFMPEG_LIB}/src
    BINARY_DIR        ${LOCAL_FFMPEG_LIB}/build
    GIT_REPOSITORY      https://git.ffmpeg.org/ffmpeg.git
    GIT_SHALLOW       ON
    UPDATE_DISCONNECTED   ON
    STEP_TARGETS      update
    CONFIGURE_COMMAND     ${LOCAL_FFMPEG_LIB}/src/configure ${FFMPEG_BUILD_OPTIONS}
    PREFIX          ${CMAKE_BINARY_DIR}
    INSTALL_DIR       ${CMAKE_BINARY_DIR}
    BUILD_COMMAND       make
    BUILD_IN_SOURCE     OFF
    INSTALL_COMMAND     make install
  )

  list(APPEND TARGET_DEPENDENCIES FFmpeg)

  set(FFMPEG_LIBS ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avcodec${CMAKE_SHARED_LIBRARY_SUFFIX}
  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avformat${CMAKE_SHARED_LIBRARY_SUFFIX}
  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avutil${CMAKE_SHARED_LIBRARY_SUFFIX}
  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}swresample${CMAKE_SHARED_LIBRARY_SUFFIX}
  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}swscale${CMAKE_SHARED_LIBRARY_SUFFIX})
  set(FFMPEG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)
endif()

add_library(miniaudio OBJECT ./lib/miniaudio/miniaudio.c)
target_include_directories(miniaudio PRIVATE ./lib/miniaudio)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

add_library(NaturalSort INTERFACE)
target_include_directories(NaturalSort INTERFACE ./lib/NaturalSort)

add_library(SPSCQueue INTERFACE)
target_include_directories(SPSCQueue INTERFACE ./lib/SPSCQueue)

add_library(readerwriterqueue INTERFACE)
target_include_directories(readerwriterqueue INTERFACE ./lib/readerwriterqueue)

configure_file(${CMAKE_SOURCE_DIR}/include/version.h.in ${CMAKE_BINARY_DIR}/include/version.h @ONLY)

if(FIND_CURSES)
  message("Using Curses Global Configuration")
endif()

message("\nCurses Libraries:")
printlist("${CURSES_LIBRARIES}")
message("\nCurses Include Directories:")
printlist("${CURSES_INCLUDE_DIR}")

if(NOT FIND_FFMPEG)
  message("\nFFmpeg Libraries:")
  printlist("${FFMPEG_LIBS}")
  message("\nFFmpeg Include Directories:")
  printlist("${FFMPEG_INCLUDE_DIR}")
else()
  message("\nUsing FFmpeg Global Configuration")
endif()

target_compile_features(tmedia PRIVATE cxx_std_17)
target_include_directories(tmedia PRIVATE ${FFMPEG_INCLUDE_DIR} ${CURSES_INCLUDE_DIR} ./include ./lib/miniaudio ${CMAKE_CURRENT_BINARY_DIR}/include )
target_link_libraries(tmedia PRIVATE ${FFMPEG_LIBS} ${CURSES_LIBRARIES} $<TARGET_OBJECTS:miniaudio> argparse NaturalSort SPSCQueue readerwriterqueue ${CMAKE_DL_LIBS})
target_compile_options(tmedia PRIVATE ${TMEDIA_COMPILE_OPTIONS})
if (NOT ${TARGET_DEPENDENCIES} STREQUAL "")
  add_dependencies(tmedia ${TARGET_DEPENDENCIES})
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
  target_include_directories(tmedia_tests PRIVATE ${FFMPEG_INCLUDE_DIR} ${CURSES_INCLUDE_DIR} ./include ./lib/miniaudio)
  target_compile_features(tmedia_tests PRIVATE cxx_std_17)
  target_link_libraries(tmedia_tests PRIVATE Catch2::Catch2WithMain ${FFMPEG_LIBS} ${CURSES_LIBRARIES} PkgConfig::LIBAV $<TARGET_OBJECTS:miniaudio> SPSCQueue readerwriterqueue ${CMAKE_DL_LIBS})

  if (NOT ${TARGET_DEPENDENCIES} STREQUAL "")
    add_dependencies(tmedia_tests ${TARGET_DEPENDENCIES})
  endif()
else()
  message("Configured to NOT build testing executable")
endif()
