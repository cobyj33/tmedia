cmake_minimum_required(VERSION 2.8.12)
# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED 17)

# if(NOT CMAKE_BUILD_TYPE)
#   set(CMAKE_BUILD_TYPE Release)
# endif()

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
# set(CMAKE_CXX_FLAGS_DEBUG "-g")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3")

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
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "Set to ON to build docs" FORCE)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "Set to ON to build examples" FORCE)
add_subdirectory(lib/FTXUI)

include_directories( ${CURSES_INCLUDE_DIR} ./includes ./lib/miniaudio/extras/miniaudio_split ./lib/argparse/include/argparse )
# include_directories( ./includes ./lib/miniaudio/extras/miniaudio_split ./lib/argparse/include/argparse )

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
./src/threads.cpp
./src/video_thread.cpp
./src/formatting.cpp
./src/wmath.cpp
./src/renderer.cpp
./src/loader.cpp
./src/wtime.cpp
./src/color.cpp
./src/icons.cpp
./src/playback.cpp
./src/pixeldata.cpp
./src/aabb.cpp
./src/scale.cpp
./src/ui/gui.cpp
)

target_link_libraries( ascii_video 
    PkgConfig::LIBAV
    ${CURSES_LIBRARIES}
    ftxui::screen
    ftxui::dom
    ftxui::component
)

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
./src/threads.cpp
./src/video_thread.cpp
./src/formatting.cpp
./src/wmath.cpp
./src/renderer.cpp
./src/loader.cpp
./src/wtime.cpp
./src/color.cpp
./src/icons.cpp
./src/playback.cpp
./src/pixeldata.cpp
./src/aabb.cpp
./src/scale.cpp
./src/ui/gui.cpp


./src/tests/test_audiostream.cpp
./src/tests/test_playback.cpp
./src/tests/test_wmath.cpp
./src/tests/test_scale.cpp
./src/tests/test_playheadlist.cpp
./src/tests/test_pixeldata.cpp
./src/tests/test_color.cpp
./src/tests/test_formatting.cpp
./src/tests/test_cmdparse.cpp
./src/tests/test_media.cpp
)

target_link_libraries(tests 
    PRIVATE Catch2::Catch2WithMain 
    ${CURSES_LIBRARIES}
    PkgConfig::LIBAV
    ftxui::screen
    ftxui::dom
    ftxui::component
)