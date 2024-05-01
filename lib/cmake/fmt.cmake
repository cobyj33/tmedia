# {fmt} cmake usage docs: https://fmt.dev/latest/usage.html#usage-with-cmake

if (FIND_FMT)
  find_package(fmt)
  list(APPEND TMEDIA_DEPS_LIBRARIES fmt::fmt)
else()
  set(TMEDIA_FMT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/fmt)

  if (NOT EXISTS ${TMEDIA_FMT_SOURCE_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "{fmt} source not found in ${TMEDIA_FMT_SOURCE_DIR}. Run ```git submodule update --init``` in the command line from the tmedia project root directory to clone the {fmt} source")
  endif()
  
  add_subdirectory(${TMEDIA_FMT_SOURCE_DIR})
  list(APPEND TMEDIA_DEPS_LIBRARIES fmt::fmt)
endif()