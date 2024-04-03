if (FIND_FMT)
  find_package(fmt)
  list(APPEND TMEDIA_DEPS_LIBRARIES fmt::fmt)
else()
  # Note that the fmt submodule has to be cloned in order for this to work
  add_subdirectory(${CMAKE_SOURCE_DIR}/lib/fmt)
  list(APPEND TMEDIA_DEPS_LIBRARIES fmt::fmt)
endif()