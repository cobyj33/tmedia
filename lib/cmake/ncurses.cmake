if(FIND_CURSES) # Default
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(NCURSES_NEED_DEBUG ON)
  endif()
  
  set(CURSES_NEED_WIDE ON)
  find_package(Curses)

  if (NOT CURSES_FOUND)
    set(CURSES_NEED_WIDE OFF)
    find_package(Curses REQUIRED)
  endif()

  list(APPEND TMEDIA_DEPS_LIBRARIES ${CURSES_LIBRARIES})
  list(APPEND TMEDIA_DEPS_INCLUDE_DIRS ${CURSES_INCLUDE_DIRS})
else()

  set(NCURSES_CONFIGURE_OPTIONS --prefix=${CMAKE_BINARY_DIR}
  --datadir=${CMAKE_BINARY_DIR}/data
  --without-tests
  --without-manpages
  --without-progs
  --without-debug
  --enable-widec
  --with-shared
  --without-normal)

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND NCURSES_CONFIGURE_OPTIONS --with-debug)
  endif()

  list(APPEND NCURSES_CONFIGURE_OPTIONS ${NCURSES_EXTRA_CONFIGURE_OPTIONS})

  message("\nNCurses Configure Options:")
  printlist("${NCURSES_CONFIGURE_OPTIONS}")

  ExternalProject_Add(
    ncurses6
    SOURCE_DIR    ${TMEDIA_DEPS_DOWNLOAD_DIR}/ncurses/src
    BINARY_DIR    ${TMEDIA_DEPS_DOWNLOAD_DIR}/ncurses/build
    PREFIX      ${CMAKE_BINARY_DIR}
    INSTALL_DIR     ${CMAKE_BINARY_DIR}
    CONFIGURE_COMMAND ${TMEDIA_DEPS_DOWNLOAD_DIR}/ncurses/src/configure ${NCURSES_CONFIGURE_OPTIONS}
    BUILD_COMMAND   make -j
    INSTALL_COMMAND   make install
    BUILD_IN_SOURCE   OFF
    URL         https://ftp.gnu.org/pub/gnu/ncurses/ncurses-6.4.tar.gz
    TLS_VERIFY    ON
  )

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CURSES_LIBRARIES
        ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}ncursesw_g${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}formw_g${CMAKE_SHARED_LIBRARY_SUFFIX})
  else()
    set(CURSES_LIBRARIES
      ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}ncursesw${CMAKE_SHARED_LIBRARY_SUFFIX}
      ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}formw${CMAKE_SHARED_LIBRARY_SUFFIX})
  endif()

  set(CURSES_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/include/ncurses)
  list(APPEND TMEDIA_DEPS_LIBRARIES ${CURSES_LIBRARIES})
  list(APPEND TMEDIA_DEPS_INCLUDE_DIRS ${CURSES_INCLUDE_DIRS})
  list(APPEND TMEDIA_TARGET_DEPENDENCIES ncurses6)
endif()