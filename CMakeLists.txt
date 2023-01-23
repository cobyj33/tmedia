cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED 11)

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

include_directories( ${CURSES_INCLUDE_DIR} ./includes )

aux_source_directory(./src SRC)
add_executable(ascii_video  ${SRC})
target_link_libraries( ascii_video ${CURSES_LIBRARIES} PkgConfig::LIBAV)
