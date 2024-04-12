if(FIND_FFMPEG)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET libavcodec
  libavformat
  libavutil
  libswresample
  libswscale)

  set(FFMPEG_LIBS PkgConfig::LIBAV)
  list(APPEND TMEDIA_DEPS_LIBRARIES ${FFMPEG_LIBS})
else()
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU"))
    list(APPEND FFMPEG_CONFIGURE_OPTIONS --extra-cflags=-g)
  endif()

  ExternalProject_Add(
    libogg
    SOURCE_DIR        ${TMEDIA_DEPS_DOWNLOAD_DIR}/ogg/src
    BINARY_DIR        ${TMEDIA_DEPS_DOWNLOAD_DIR}/ogg/build
    URL http://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.gz
    URL_HASH SHA256=0eb4b4b9420a0f51db142ba3f9c64b333f826532dc0f48c6410ae51f4799b664
    CMAKE_ARGS ${TMEDIA_DEPS_CMAKE_PROFILE} ${LIBOGG_EXTRA_CMAKE_ARGS} 
    UPDATE_COMMAND ""
  )

  ExternalProject_Add(
    libvorbis
    SOURCE_DIR        ${TMEDIA_DEPS_DOWNLOAD_DIR}/vorbis/src
    BINARY_DIR        ${TMEDIA_DEPS_DOWNLOAD_DIR}/vorbis/build
    URL http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.xz
    URL_HASH SHA256=b33cc4934322bcbf6efcbacf49e3ca01aadbea4114ec9589d1b1e9d20f72954b
    CMAKE_ARGS ${TMEDIA_DEPS_CMAKE_PROFILE} ${LIBVORBIS_EXTRA_CMAKE_ARGS} 
    UPDATE_COMMAND ""
    DEPENDS libogg
  )

  ExternalProject_Add(
    libopus
    SOURCE_DIR        ${TMEDIA_DEPS_DOWNLOAD_DIR}/opus/src
    BINARY_DIR        ${TMEDIA_DEPS_DOWNLOAD_DIR}/opus/build
    GIT_REPOSITORY https://gitlab.xiph.org/xiph/opus.git
    GIT_TAG v1.3.1
    CMAKE_ARGS ${TMEDIA_DEPS_CMAKE_PROFILE} -DOPUS_STACK_PROTECTOR=OFF
    UPDATE_COMMAND ""
  )

  list(APPEND FFMPEG_CONFIGURE_OPTIONS ${FFMPEG_EXTRA_CONFIGURE_OPTIONS})
  message("\nFFMPEG Configure Options:")
  printlist("${FFMPEG_CONFIGURE_OPTIONS}")

  ExternalProject_Add(
    FFmpeg
    SOURCE_DIR        ${TMEDIA_DEPS_DOWNLOAD_DIR}/ffmpeg/src
    BINARY_DIR        ${TMEDIA_DEPS_DOWNLOAD_DIR}/ffmpeg/build
    GIT_REPOSITORY      https://git.ffmpeg.org/ffmpeg.git
    GIT_SHALLOW       ON
    UPDATE_DISCONNECTED   ON
    STEP_TARGETS      update
    CONFIGURE_COMMAND     ${TMEDIA_DEPS_DOWNLOAD_DIR}/ffmpeg/src/configure --enable-libvorbis --enable-libopus ${FFMPEG_CONFIGURE_OPTIONS}
    PREFIX          ${CMAKE_BINARY_DIR}
    INSTALL_DIR       ${CMAKE_BINARY_DIR}
    BUILD_COMMAND       make -j${TMEDIA_DEP_JOBS}
    BUILD_IN_SOURCE     OFF
    INSTALL_COMMAND     make install
    DEPENDS libvorbis libopus
  )
  
  
  set(FFMPEG_LIBS ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avcodec${CMAKE_SHARED_LIBRARY_SUFFIX}
  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avformat${CMAKE_SHARED_LIBRARY_SUFFIX}
  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}avutil${CMAKE_SHARED_LIBRARY_SUFFIX}
  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}swresample${CMAKE_SHARED_LIBRARY_SUFFIX}
  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}swscale${CMAKE_SHARED_LIBRARY_SUFFIX})
  set(FFMPEG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)
  
  list(APPEND TMEDIA_DEPS_LIBRARIES ${FFMPEG_LIBS})
  list(APPEND TMEDIA_DEPS_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
  list(APPEND TMEDIA_TARGET_DEPENDENCIES FFmpeg)
endif()