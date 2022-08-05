cmake_minimum_required(VERSION 2.8)
project( ascii_video )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package( OpenCV REQUIRED )
find_package(Curses REQUIRED)
find_package(SDL2 REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS} ${CURSES_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS} ./includes )

aux_source_directory(./src SRC)
add_executable(ascii_video  ${SRC})
target_link_libraries( ascii_video ${OpenCV_LIBS} ${CURSES_LIBRARIES} ${SDL2_LIBRARIES} )
