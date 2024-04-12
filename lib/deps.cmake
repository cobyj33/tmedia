# Note that this file should be called with include instead of add_subdirectory,
# since add_subdirectory will create a new scope, and include will not. We'd
# like all variables declared and modified  here and in the cmake modules
# directories to be exposed to the top-level CMakeLists.txt 

# prefer local modules to cmake global modules (FindCurses)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/lib/cmake/modules)

set(TMEDIA_DEPS_CMAKE_PROFILE 
-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
-DBUILD_SHARED_LIBS=ON
-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}
-DBUILD_PROGRAMS=OFF
-DBUILD_EXAMPLES=OFF
-DBUILD_TESTING=OFF
-DBUILD_DOCS=OFF
-DINSTALL_MANPAGES=OFF)

add_library(miniaudio ${CMAKE_SOURCE_DIR}/lib/miniaudio/miniaudio.c)
target_include_directories(miniaudio PUBLIC ${CMAKE_SOURCE_DIR}/lib/miniaudio)
list(APPEND TMEDIA_DEPS_LIBRARIES miniaudio)

add_library(NaturalSort INTERFACE)
target_include_directories(NaturalSort INTERFACE ${CMAKE_SOURCE_DIR}/lib/NaturalSort)
list(APPEND TMEDIA_DEPS_LIBRARIES NaturalSort)

add_library(readerwriterqueue INTERFACE)
target_include_directories(readerwriterqueue INTERFACE ${CMAKE_SOURCE_DIR}/lib/readerwriterqueue)
list(APPEND TMEDIA_DEPS_LIBRARIES readerwriterqueue)

add_library(random INTERFACE)
target_include_directories(random INTERFACE ${CMAKE_SOURCE_DIR}/lib/random)
list(APPEND TMEDIA_DEPS_LIBRARIES random)

include(${CMAKE_SOURCE_DIR}/lib/cmake/ffmpeg.cmake)
include(${CMAKE_SOURCE_DIR}/lib/cmake/ncurses.cmake)
include(${CMAKE_SOURCE_DIR}/lib/cmake/fmt.cmake)

