cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#Used in configuring version file
set(ASCII_VIDEO_VERSION_MAJOR 0)
set(ASCII_VIDEO_VERSION_MINOR 4)
set(ASCII_VIDEO_VERSION_PATCH 0)
set(ASCII_VIDEO_VERSION "${ASCII_VIDEO_VERSION_MAJOR}.${ASCII_VIDEO_VERSION_MINOR}.${ASCII_VIDEO_VERSION_PATCH}")

set(LOCAL_NCURSES_LIB ${CMAKE_SOURCE_DIR}/lib/ncurses)
set(LOCAL_FFMPEG_LIB ${CMAKE_SOURCE_DIR}/lib/ffmpeg)

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

set(ASCII_VIDEO_SUPPORTED_LANGUAGES CXX C)
if (APPLE)
    list (APPEND ASCII_VIDEO_SUPPORTED_LANGUAGES OBJC OBJCXX)
endif()

if (WIN32)
    message(WARNING "Windows builds are completely untested as of now. Good luck")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message("No Build Type Given, setting build type to Release")
endif()
message("Building Build Type: " ${CMAKE_BUILD_TYPE})

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and run cmake from there")
endif()

option(ASCII_VIDEO_BUILD_TESTS "Build Testing Executable for ascii_video" OFF)
option(FIND_FFMPEG "Find FFmpeg Libraries on system rather than build FFmpeg alongside ascii_video" OFF)
option(FIND_CURSES "Find Curses Libraries on system rather than build NCurses alongside ascii_video" OFF)
option(FFMPEG_ENABLE_GPL "Enable GPL Licensing on the built FFmpeg libraries" OFF)
option(FFMPEG_ENABLE_NONFREE "Enable Proprietary Licensing on the built FFmpeg libraries" OFF)
option(FFMPEG_ENABLE_LGPL_THREE "Enable LGPL3 Licensing on the built FFmpeg Libraries" OFF)
option(FFMPEG_DISABLE_AUTODETECT "Disable FFmpeg from autodetecting utility libraries and hardware accelerators" OFF)
option(ADD_GDB_FLAG_ON_CBTD "Compile dependencies with the -g flag for gdb if CMAKE_BUILD_TYPE is Debug and the compiler is gcc or clang" ON)
option(DEPS_ALREADY_COMPILED "Do not recompile dependencies because they have already been compiled and are available in the build directory" OFF)

message("Options: ASCII_VIDEO_BUILD_TESTS: " ${ASCII_VIDEO_BUILD_TESTS})
message("Options: FIND_FFMPEG: " ${FIND_FFMPEG})
message("Options: FIND_CURSES: " ${FIND_CURSES})
message("Options: FFMPEG_ENABLE_GPL: " ${FFMPEG_ENABLE_GPL})
message("Options: FFMPEG_ENABLE_NONFREE: " ${FFMPEG_ENABLE_NONFREE})
message("Options: FFMPEG_ENABLE_LGPL_THREE: " ${FFMPEG_ENABLE_LGPL_THREE})
message("Options: ADD_GDB_FLAG_ON_CBTD: " ${ADD_GDB_FLAG_ON_CBTD})
message("Options: FFMPEG_DISABLE_AUTODETECT: " ${FFMPEG_DISABLE_AUTODETECT})
message("Options: DEPS_ALREADY_COMPILED: " ${DEPS_ALREADY_COMPILED})

set(NCURSES_EXTRA_CONFIGURE_FLAGS "")
set(FFMPEG_EXTRA_CONFIGURE_FLAGS "")

project(ascii_video
VERSION ${ASCII_VIDEO_VERSION}
LANGUAGES ${ASCII_VIDEO_SUPPORTED_LANGUAGES}
DESCRIPTION "Terminal video media player")

add_executable(ascii_video ${COMMON_SOURCE_FILES} ./src/main.cpp)

set(NCURSES_CONFIGURE_OPTIONS --prefix=${CMAKE_BINARY_DIR}
--datadir=${CMAKE_BINARY_DIR}/data
--without-tests
--without-manpages
--without-progs
--with-shared
--without-normal
--without-debug
)

include(ExternalProject)

if(FIND_CURSES)
    find_package(Curses REQUIRED)
elseif(NOT FIND_CURSES AND (UNIX OR APPLE OR MINGW OR CYGWIN))

    if(NOT DEPS_ALREADY_COMPILED)
        if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU") AND ADD_GDB_FLAG_ON_CBTD)
            message("Appending -g flag to ncurses configure cflags")
            list(APPEND NCURSES_CONFIGURE_OPTIONS --with-build-cflags=-g)
        elseif(CMAKE_BUILD_TYPE STREQUAL "Debug" AND (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU") AND NOT ADD_GDB_FLAG_ON_CBTD)
            message("Not appending -g flag to ncurses configure cflags: gdb debugging build of ncurses has been disabled through the ADD_GDB_FLAG_ON_CBTD option")
        endif()

        if(NOT NCURSES_EXTRA_CONFIGURE_FLAGS STREQUAL "")
            message("Appending extra ncurses configure flags: ${NCURSES_EXTRA_CONFIGURE_FLAGS}")
            list(APPEND NCURSES_CONFIGURE_OPTIONS ${NCURSES_EXTRA_CONFIGURE_FLAGS})
        endif()

        message("Curses configure options: ${NCURSES_CONFIGURE_OPTIONS}")
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
    endif()

    set(CURSES_LIBRARIES ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}ncurses${CMAKE_SHARED_LIBRARY_SUFFIX}
    ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}form${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CURSES_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include/ncurses)
else()
    message(WARNING "Currently, Non-Unix builds are completely untested. Good luck.")
endif()

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

if(FIND_FFMPEG)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET libavcodec
    libavformat
    libavutil
    libswresample
    libswscale
    )

    target_link_libraries(ascii_video PRIVATE PkgConfig::LIBAV)
else()
    if(NOT DEPS_ALREADY_COMPILED)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU") AND ADD_GDB_FLAG_ON_CBTD)
            message("Appending -g flag to FFmpeg configure cflags")
            list(APPEND FFMPEG_BUILD_OPTIONS --extra-cflags=-g)
        elseif(CMAKE_BUILD_TYPE STREQUAL "Debug" AND (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU") AND NOT ADD_GDB_FLAG_ON_CBTD)
            message("Not appending -g flag to FFmpeg configure cflags: gdb debugging build of FFmpeg has been disabled through the ADD_GDB_FLAG_ON_CBTD option")
        endif()

        if(FFMPEG_ENABLE_GPL) 
            message("Appending --enable-gpl to FFmpeg configure options")
            list(APPEND FFMPEG_BUILD_OPTIONS --enable-gpl)
        endif()

        if(FFMPEG_ENABLE_LGPL_THREE)
            message("Appending --enable-version3 to FFmpeg configure options")
            list(APPEND FFMPEG_BUILD_OPTIONS --enable-version3)
        endif()

        if(FFMPEG_ENABLE_NONFREE)
            message("Appending --enable-nonfree to FFmpeg configure options")
            list(APPEND FFMPEG_BUILD_OPTIONS --enable-nonfree)
        endif()

        if(FFMPEG_DISABLE_AUTODETECT)
            message("Appending --disable-autodetect to FFMpeg configure options")
            list(APPEND FFMPEG_BUILD_OPTIONS --disable-autodetect)
        endif()

        if(NOT FFMPEG_EXTRA_CONFIGURE_FLAGS STREQUAL "")
            message("Appending extra FFmpeg configure flags: ${FFMPEG_EXTRA_CONFIGURE_FLAGS}")
            list(APPEND FFMPEG_BUILD_OPTIONS ${FFMPEG_EXTRA_CONFIGURE_FLAGS})
        endif()

        message("FFMPEG Build Options: ${FFMPEG_BUILD_OPTIONS}")
        ExternalProject_Add(
            FFmpeg
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
    endif()

    set(FFMPEG_LIBS ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avcodec${CMAKE_SHARED_LIBRARY_SUFFIX}
    ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avformat${CMAKE_SHARED_LIBRARY_SUFFIX}
    ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avutil${CMAKE_SHARED_LIBRARY_SUFFIX}
    ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}swresample${CMAKE_SHARED_LIBRARY_SUFFIX}
    ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}swscale${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(FFMPEG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)
endif()


add_library(miniaudio OBJECT ./lib/miniaudio/miniaudio.c)
target_include_directories(miniaudio PRIVATE ./lib/miniaudio)

add_library(argparse INTERFACE)
target_include_directories(argparse INTERFACE ./lib/argparse)

configure_file(${CMAKE_SOURCE_DIR}/include/version.h.in ${CMAKE_BINARY_DIR}/include/version.h @ONLY)

message("Curses Libraries: ${CURSES_LIBRARIES}")
message("Curses Include Directories: ${CURSES_INCLUDE_DIR}")
message("FFmpeg Libraries: ${FFMPEG_LIBS}")
message("FFmpeg Include Directories: ${FFMPEG_INCLUDE_DIR}")

target_compile_features(ascii_video PRIVATE cxx_std_17)
target_include_directories(ascii_video PRIVATE ${FFMPEG_INCLUDE_DIR} ${CURSES_INCLUDE_DIR} ./include ./lib/miniaudio ${CMAKE_CURRENT_BINARY_DIR}/include )
target_link_libraries(ascii_video PRIVATE ${FFMPEG_LIBS} ${CURSES_LIBRARIES} $<TARGET_OBJECTS:miniaudio> argparse ${CMAKE_DL_LIBS})

if(NOT FIND_FFMPEG AND NOT DEPS_ALREADY_COMPILED)
    add_dependencies(ascii_video FFmpeg)
endif()

if(NOT FIND_CURSES AND NOT DEPS_ALREADY_COMPILED)
    add_dependencies(ascii_video ncurses6)
endif()

set(ASCII_VIDEO_RPATH ${CMAKE_BINARY_DIR}/lib /usr/local/lib)
set_target_properties(ascii_video PROPERTIES INSTALL_RPATH "'$ORIGIN'/lib;${ASCII_VIDEO_RPATH}")
set_target_properties(ascii_video PROPERTIES BUILD_WITH_INSTALL_RPATH ON)
# target_compile_options(ascii_video PRIVATE -Wl,-rpath,'$ORIGIN'/lib)

if(MSVC)
    target_compile_options(ascii_video PRIVATE /std:c++17 /W4 /WX)
else()
    target_compile_options(ascii_video PRIVATE -Wall -Wextra -Wpedantic) # Consider adding -Werror back
endif()

if (ASCII_VIDEO_BUILD_TESTS)
    message("Configured to build testing executable")
    Include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.0.1 # or a later release
    )

    FetchContent_MakeAvailable(Catch2)

    set(TEST_SOURCE_FILES
    ./src/tests/test_audiobuffer.cpp
    ./src/tests/test_playback.cpp
    ./src/tests/test_wmath.cpp
    ./src/tests/test_scale.cpp
    ./src/tests/test_pixeldata.cpp
    ./src/tests/test_color.cpp
    ./src/tests/test_formatting.cpp
    )

    add_executable(ascii_tests ${COMMON_SOURCE_FILES} ${TEST_SOURCE_FILES})

    if(MSVC)
        target_compile_options(ascii_tests PRIVATE /std:c++17 /W4 /WX)
    else()
        target_compile_options(ascii_tests PRIVATE -Wall -Wextra -Wpedantic)# Consider adding -Werror back
    endif()

    target_include_directories(ascii_tests PRIVATE ${FFMPEG_INCLUDE_DIR} ${CURSES_INCLUDE_DIR} ./include ./lib/miniaudio)
    target_compile_features(ascii_tests PRIVATE cxx_std_17)
    target_link_libraries(ascii_tests PRIVATE Catch2::Catch2WithMain ${FFMPEG_LIBS} ${CURSES_LIBRARIES} PkgConfig::LIBAV $<TARGET_OBJECTS:miniaudio> ${CMAKE_DL_LIBS})
    # set_target_properties(ascii_tests PROPERTIES INSTALL_RPATH ${CMAKE_BINARY_DIR}/lib)
    # set_target_properties(ascii_tests PROPERTIES BUILD_WITH_INSTALL_RPATH ON)

    if(NOT FIND_FFMPEG AND NOT DEPS_ALREADY_COMPILED)
        add_dependencies(ascii_tests FFmpeg)
    endif()

    if(NOT FIND_CURSES AND NOT DEPS_ALREADY_COMPILED)
        add_dependencies(ascii_tests ncurses6)
    endif()
else()
    message("Configured to NOT build testing executable")
endif()
