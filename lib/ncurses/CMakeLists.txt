# Responsible for building the ncurses libs into the lib/build directory

# LOCAL_NCURSES_LIB defined at lib/CMakeLists.txt

set(NCURSES_BUILD_DIR ${LOCAL_NCURSES_LIB}/build)
include(ExternalProject)

ExternalProject_Add(
    ncurses6
    SOURCE_DIR        ${LOCAL_NCURSES_LIB}
    BINARY_DIR        ${NCURSES_BUILD_DIR}
    CONFIGURE_COMMAND ${LOCAL_NCURSES_LIB}/ncurses-6.4/configure --prefix=${NCURSES_BUILD_DIR} --datadir=${NCURSES_BUILD_DIR}/data --without-tests --without-manpages --without-progs
    BUILD_COMMAND     "make -j${SUBMAKE_JOBS}"
    INSTALL_COMMAND   make install
    BUILD_IN_SOURCE   OFF
    URL               ${LOCAL_NCURSES_LIB}/ncurses-6.4.tar.gz
)