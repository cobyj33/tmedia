cmake_minimum_required(VERSION 2.8)
project( ascii_video )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package( OpenCV REQUIRED )
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

include_directories( ${OpenCV_INCLUDE_DIRS} ${CURSES_INCLUDE_DIR} ./includes )

aux_source_directory(./src SRC)
add_executable(ascii_video  ${SRC})
target_link_libraries( ascii_video ${OpenCV_LIBS} ${CURSES_LIBRARIES} PkgConfig::LIBAV)
