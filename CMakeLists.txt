cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ASCII_VIDEO_VERSION_MAJOR 0)
set(ASCII_VIDEO_VERSION_MINOR 4)
set(ASCII_VIDEO_VERSION_PATCH 0)
set(ASCII_VIDEO_VERSION "${ASCII_VIDEO_VERSION_MAJOR}.${ASCII_VIDEO_VERSION_MINOR}.${ASCII_VIDEO_VERSION_PATCH}")

set(LOCAL_NCURSES_LIB ${CMAKE_SOURCE_DIR}/lib/ncurses)
set(LOCAL_FFMPEG_LIB ${CMAKE_SOURCE_DIR}/lib/ffmpeg)

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
VERSION ${ASCII_VIDEO_VERSION}
LANGUAGES ${ASCII_VIDEO_SUPPORTED_LANGUAGES}
DESCRIPTION "Terminal video media player")

option(ASCII_VIDEO_BUILD_TESTS "Build Testing Executable for ascii_video" OFF)

if (ASCII_VIDEO_BUILD_TESTS)
    message("Configured to build testing executable")
else()
    message("Configured to NOT build testing executable")
endif()

set(NCURSES_CONFIGURE_OPTIONS --prefix=${CMAKE_BINARY_DIR}
--datadir=${CMAKE_BINARY_DIR}/data
--without-tests
--without-manpages
--without-progs
--with-shared
--without-normal
--without-debug
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND NCURSES_CONFIGURE_OPTIONS --with-build-cflags=-g)
endif()

include(ExternalProject)
ExternalProject_Add(
    ncurses6
    SOURCE_DIR        ${LOCAL_NCURSES_LIB}/src
    BINARY_DIR        ${LOCAL_NCURSES_LIB}/build
    PREFIX            ${CMAKE_BINARY_DIR}
    INSTALL_DIR       ${CMAKE_BINARY_DIR}
    CONFIGURE_COMMAND ${LOCAL_NCURSES_LIB}/src/configure ${NCURSES_CONFIGURE_OPTIONS}
    BUILD_COMMAND     make
    INSTALL_COMMAND   make install
    BUILD_IN_SOURCE   OFF
    URL               https://ftp.gnu.org/pub/gnu/ncurses/ncurses-6.4.tar.gz
    TLS_VERIFY        ON
)

set(CURSES_LIBRARIES ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}ncurses${CMAKE_SHARED_LIBRARY_SUFFIX}
${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}form${CMAKE_SHARED_LIBRARY_SUFFIX})

set(CURSES_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include/ncurses)

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
--enable-protocol=file,pipe)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND FFMPEG_BUILD_OPTIONS --extra-cflags=-g)
endif()
# --enable-libdav1d
# --enable-libvorbis
# --ld="g++"
# --disable-shared
# --enable-static

ExternalProject_Add(
    ffmpeg
    SOURCE_DIR              ${LOCAL_FFMPEG_LIB}/src
    BINARY_DIR              ${LOCAL_FFMPEG_LIB}/build
    GIT_REPOSITORY          https://git.ffmpeg.org/ffmpeg.git
    GIT_SHALLOW             ON
    UPDATE_DISCONNECTED     ON
    STEP_TARGETS            update
    CONFIGURE_COMMAND       ${LOCAL_FFMPEG_LIB}/src/configure ${FFMPEG_BUILD_OPTIONS}
    PREFIX                  ${CMAKE_BINARY_DIR}
    INSTALL_DIR             ${CMAKE_BINARY_DIR}
    BUILD_COMMAND           make
    BUILD_IN_SOURCE         OFF
    INSTALL_COMMAND         make install
)

set(PKG_CONFIG_PATH_SEPARATOR :)
if (WIN32)
    set(PKG_CONFIG_PATH_SEPARATOR ;)
endif()

# set(ENV{PKG_CONFIG_PATH} ${CMAKE_BINARY_DIR}/lib/pkgconfig${PKG_CONFIG_PATH_SEPARATOR}$ENV{PKG_CONFIG_PATH})
set(FFMPEG_LIBS ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avcodec${CMAKE_SHARED_LIBRARY_SUFFIX}
${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avformat${CMAKE_SHARED_LIBRARY_SUFFIX}
${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avutil${CMAKE_SHARED_LIBRARY_SUFFIX}
${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}swresample${CMAKE_SHARED_LIBRARY_SUFFIX}
${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}swscale${CMAKE_SHARED_LIBRARY_SUFFIX})
set(FFMPEG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)
# set(ENV{PKG_CONFIG_PATH} ${CMAKE_BINARY_DIR}/lib/pkgconfig)
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(
#     LIBAV REQUIRED IMPORTED_TARGET
#     libavformat
#     libavcodec
#     libswresample
#     libswscale
#     libavutil
# )

# if(NOT USE_SHARED_ALL)
#     message("Experimental Library Building Enabled")
#     # add_subdirectory(${CMAKE_SOURCE_DIR}/lib)
# else()
#     find_package(Curses REQUIRED)
#     find_package(PkgConfig REQUIRED)
#     pkg_check_modules(
#         LIBAV REQUIRED IMPORTED_TARGET
#         libavformat
#         libavcodec
#         libswresample
#         libswscale
#         libavutil
#     )
# endif()

if(ASCII_VIDEO_BUILD_TESTS)
    add_subdirectory(./lib/Catch2)
endif()

add_library(miniaudio OBJECT ./lib/miniaudio/miniaudio.c)
target_include_directories(miniaudio PRIVATE ./lib/miniaudio)

add_library(argparse INTERFACE)
target_include_directories(argparse INTERFACE ./lib/argparse)

configure_file(${CMAKE_SOURCE_DIR}/include/version.h.in ${CMAKE_BINARY_DIR}/include/version.h @ONLY)
    
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
target_include_directories(ascii_video PRIVATE ${FFMPEG_INCLUDE_DIR} ${CURSES_INCLUDE_DIR} ./include ./lib/miniaudio ${CMAKE_CURRENT_BINARY_DIR}/include )
target_link_libraries(ascii_video PRIVATE ${FFMPEG_LIBS} ${CURSES_LIBRARIES} $<TARGET_OBJECTS:miniaudio> argparse ${CMAKE_DL_LIBS})
add_dependencies(ascii_video ffmpeg ncurses6)
set_target_properties(ascii_video PROPERTIES INSTALL_RPATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(ascii_video PROPERTIES BUILD_WITH_INSTALL_RPATH ON)
target_compile_options(ascii_video PRIVATE -Wl,-rpath,'$ORIGIN'/lib)

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
    add_dependencies(tests ffmpeg ncurses6)
    target_include_directories(ascii_tests PRIVATE ${FFMPEG_INCLUDE_DIR} ${CURSES_INCLUDE_DIR} ./include ./lib/miniaudio)
    target_compile_features(ascii_tests PRIVATE cxx_std_17)
    target_link_libraries(ascii_tests PRIVATE Catch2::Catch2WithMain ${FFMPEG_LIBS} ${CURSES_LIBRARIES} PkgConfig::LIBAV $<TARGET_OBJECTS:miniaudio> ${CMAKE_DL_LIBS})
    set_target_properties(ascii_tests PROPERTIES INSTALL_RPATH ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(ascii_tests PROPERTIES BUILD_WITH_INSTALL_RPATH ON)
endif()