cmake_minimum_required(VERSION 2.8)
project( ascii_video )
find_package( OpenCV REQUIRED )
find_package(Curses REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS} ${CURSES_INCLUDE_DIR} )
add_executable( ascii_video main.cpp )
target_link_libraries( ascii_video ${OpenCV_LIBS} ${CURSES_LIBRARIES} )