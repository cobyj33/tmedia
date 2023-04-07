
if (NOT SUBMAKE_JOBS)
    include(ProcessorCount)
    ProcessorCount(NUM_CORES)
    if  (NOT NUM_CORES EQUAL 0)
        if (NUM_CORES GREATER 2)
            # be nice...
            MATH( EXPR NUM_CORES "${NUM_CORES} - 2" )
        endif()
        set(SUBMAKE_JOBS ${NUM_CORES})
    else()
        set(SUBMAKE_JOBS 1)
    endif()
endif()

MESSAGE("SUBMAKE_JOBS: " ${SUBMAKE_JOBS})

set(LOCAL_NCURSES_LIB ${CMAKE_SOURCE_DIR}/lib/ncurses)
set(LOCAL_FFMPEG_LIB ${CMAKE_SOURCE_DIR}/lib/ffmpeg)

if(NOT USE_SHARED_NCURSES)
    add_subdirectory(${LOCAL_NCURSES_LIB})
else()
    find_package(Curses)
    if((NOT CURSES_FOUND) AND FALLBACK_BUILD_FROM_SHARED)
        add_subdirectory(${LOCAL_NCURSES_LIB})
    endif()
endif()
    
if(NOT USE_SHARED_FFMPEG)
    add_subdirectory(${CMAKE_SOURCE_DIR}/lib/ffmpeg)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(
        LIBAV REQUIRED IMPORTED_TARGET
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
    )
endif()