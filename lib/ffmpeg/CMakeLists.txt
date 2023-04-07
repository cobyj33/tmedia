# Responsible for building the ffmpeg libs into the lib/build directory
#   SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/libantlr3c-3.1.3

# include(ExternalProject)

# set(FFMPEG_BUILD_OPTIONS --prefix="${LOCAL_FFMPEG_LIB}/build}"
# --pkg-config-flags="--static"
# --extra-cflags="-I${LOCAL_FFMPEG_LIB}/build/include"
# --extra-ldflags="-L${LOCAL_FFMPEG_LIB}/build/lib"
# --extra-libs="-lpthread -lm"
# --ld="g++"
# --disable-shared
# --enable-static
# --enable-libdav1d
# --enable-libvorbis
# --disable-network
# --disable-encoders
# --disable-muxers
# --disable-programs
# --disable-doc
# --disable-filters
# --disable-postproc
# --disable-avfilter
# --disable-avdevice
# --disable-protocols
# --enable-protocol=file,pipe
# --disable-devices)

# ExternalProject_Add(
#   ffmpeg
#   SOURCE_DIR        "${LOCAL_FFMPEG_LIB}"
#   URL               "${LOCAL_FFMPEG_LIB}/ffmpeg-6.0.tar.xz"
#   CONFIGURE_COMMAND "${LOCAL_FFMPEG_LIB}/ffmpeg-6.0/configure ${FFMPEG_BUILD_OPTIONS}"
#   PREFIX            "${LOCAL_FFMPEG_LIB}/build"
#   BUILD_COMMAND     "make -j${SUBMAKE_JOBS}"
#   BUILD_IN_SOURCE   OFF
#   INSTALL_COMMAND   "make install"
# )

find_package(PkgConfig REQUIRED)
# set(PKG_CONFIG_PATH_SAVE $ENV{PKG_CONFIG_PATH})
# set(ENV{PKG_CONFIG_PATH} ${LOCAL_FFMPEG_LIB}/build/pkgconfig)

pkg_check_modules(
    LIBAV REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

message(PkgConfig::LIBAV)
if(NOT TARGET PkgConfig::LIBAV)
    message("No target named PkgConfig::LIBAV could be found in the subdirectory")
else()
    message("Target named PkgConfig::LIBAV could be found in the subdirectory")
endif()

# set(ENV{PKG_CONFIG_PATH} ${PKG_CONFIG_PATH_SAVE})
