# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

# tmedia modifications:

#   A variable NCURSES_NEED_DEBUG has been added, which will search
#   for the debug libraries for NCurses. The user must set NCURSES_NEED_DEBUG to
#   true before calling if ncurses debug libraries should be used.

#   NCURSES_NEED_DEBUG implies ncurses should be searched for. However, if no
#   ncurses could be found, AND CURSES_NEED_CURSES is set to false by the user,
#   then this module is allowed to fallback to libcurses.

#   Note that instead of returning an error, this module will instead fallback
#   toward normal ncurses libs if debug ncurses libs could not be found. This
#   fallback will be messaged out to the terminal as well.

#   Note that I've also chosen NCURSES_NEED_DEBUG instead of CURSES_NEED_DEBUG,
#   as the only curses library that I know emits separate debug libraries is
#   ncurses.

#[=======================================================================[.rst:
FindCurses
----------

Find the curses or ncurses include file and library.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``CURSES_FOUND``
  True if Curses is found.
``CURSES_INCLUDE_DIRS``
  The include directories needed to use Curses.
``CURSES_LIBRARIES``
  The libraries needed to use Curses.
``CURSES_CFLAGS``
  .. versionadded:: 3.16

  Parameters which ought be given to C/C++ compilers when using Curses.
``CURSES_HAVE_CURSES_H``
  True if curses.h is available.
``CURSES_HAVE_NCURSES_H``
  True if ncurses.h is available.
``CURSES_HAVE_NCURSES_NCURSES_H``
  True if ``ncurses/ncurses.h`` is available.
``CURSES_HAVE_NCURSES_CURSES_H``
  True if ``ncurses/curses.h`` is available.

Set ``CURSES_NEED_NCURSES`` to ``TRUE`` before the
``find_package(Curses)`` call if NCurses functionality is required.

.. versionadded:: 3.10
  Set ``CURSES_NEED_WIDE`` to ``TRUE`` before the
  ``find_package(Curses)`` call if unicode functionality is required.

Backward Compatibility
^^^^^^^^^^^^^^^^^^^^^^

The following variable are provided for backward compatibility:

``CURSES_INCLUDE_DIR``
  Path to Curses include.  Use ``CURSES_INCLUDE_DIRS`` instead.
``CURSES_LIBRARY``
  Path to Curses library.  Use ``CURSES_LIBRARIES`` instead.
#]=======================================================================]

include(CheckLibraryExists)

# tmedia modifications start here.

if (NCURSES_NEED_DEBUG) # NCURSES_NEED_DEBUG implies CURSES_NEED_NCURSES
  set(CURSES_NEED_NCURSES TRUE)
endif()

function(set_ncurses_library_names)
  # we don't know anything about cursesw, so only ncurses
  # may be ncursesw
  if(NOT CURSES_NEED_WIDE)
    set(NCURSES_LIBRARY_NAME PARENT_SCOPE "ncurses")
    set(CURSES_FORM_LIBRARY_NAME PARENT_SCOPE "form")
  else()
    set(NCURSES_LIBRARY_NAME PARENT_SCOPE "ncursesw")
    set(CURSES_FORM_LIBRARY_NAME PARENT_SCOPE "formw")
    # Also, if we are searching for wide curses - we are actually searching
    # for ncurses, we don't know about any other unicode version.
    set(CURSES_NEED_NCURSES PARENT_SCOPE TRUE)
  endif()
endfunction()

function(set_ncurses_library_names_debug)
  # we don't know anything about cursesw, so only ncurses
  # may be ncursesw
  if(NOT CURSES_NEED_WIDE)
    set(NCURSES_LIBRARY_NAME "ncurses_g" PARENT_SCOPE)
    set(CURSES_FORM_LIBRARY_NAME "form_g" PARENT_SCOPE)
  else()
    set(NCURSES_LIBRARY_NAME "ncursesw_g" PARENT_SCOPE)
    set(CURSES_FORM_LIBRARY_NAME "formw_g" PARENT_SCOPE)
    # Also, if we are searching for wide curses - we are actually searching
    # for ncurses, we don't know about any other unicode version.
    set(CURSES_NEED_NCURSES TRUE PARENT_SCOPE)
  endif()
endfunction()

if(NCURSES_NEED_DEBUG)
  set_ncurses_library_names_debug()
else()
  set_ncurses_library_names()
endif()

find_library(CURSES_CURSES_LIBRARY NAMES curses)

find_library(CURSES_NCURSES_LIBRARY NAMES "${NCURSES_LIBRARY_NAME}" )
if ((NOT CURSES_NCURSES_LIBRARY) AND NCURSES_NEED_DEBUG) # fallback to normal ncurses libraries
  message("Could not find debug ncurses sources... falling back to normal curses")
  set_ncurses_library_names()
  find_library(CURSES_NCURSES_LIBRARY NAMES "${NCURSES_LIBRARY_NAME}")
endif()

# tmedia modifications end here. Everything under this comment is unmodified
# from the cmake source tree.

set(CURSES_USE_NCURSES FALSE)

if(CURSES_NCURSES_LIBRARY  AND ((NOT CURSES_CURSES_LIBRARY) OR CURSES_NEED_NCURSES))
  set(CURSES_USE_NCURSES TRUE)
endif()
# http://cygwin.com/ml/cygwin-announce/2010-01/msg00002.html
# cygwin ncurses stopped providing curses.h symlinks see above
# message.  Cygwin is an ncurses package, so force ncurses on
# cygwin if the curses.h is missing
if(CURSES_NCURSES_LIBRARY AND CYGWIN)
  if (CURSES_NEED_WIDE)
    if(NOT EXISTS /usr/include/ncursesw/curses.h)
      set(CURSES_USE_NCURSES TRUE)
    endif()
  else()
    if(NOT EXISTS /usr/include/curses.h)
      set(CURSES_USE_NCURSES TRUE)
    endif()
  endif()
endif()


# Not sure the logic is correct here.
# If NCurses is required, use the function wsyncup() to check if the library
# has NCurses functionality (at least this is where it breaks on NetBSD).
# If wsyncup is in curses, use this one.
# If not, try to find ncurses and check if this has the symbol.
# Once the ncurses library is found, search the ncurses.h header first, but
# some web pages also say that even with ncurses there is not always a ncurses.h:
# http://osdir.com/ml/gnome.apps.mc.devel/2002-06/msg00029.html
# So at first try ncurses.h, if not found, try to find curses.h under the same
# prefix as the library was found, if still not found, try curses.h with the
# default search paths.
if(CURSES_CURSES_LIBRARY  AND  CURSES_NEED_NCURSES)
  include(CMakePushCheckState)
  cmake_push_check_state()
  set(CMAKE_REQUIRED_QUIET ${Curses_FIND_QUIETLY})
  CHECK_LIBRARY_EXISTS("${CURSES_CURSES_LIBRARY}"
    wsyncup "" CURSES_CURSES_HAS_WSYNCUP)

  if(CURSES_NCURSES_LIBRARY  AND NOT  CURSES_CURSES_HAS_WSYNCUP)
    CHECK_LIBRARY_EXISTS("${CURSES_NCURSES_LIBRARY}"
      wsyncup "" CURSES_NCURSES_HAS_WSYNCUP)
    if( CURSES_NCURSES_HAS_WSYNCUP)
      set(CURSES_USE_NCURSES TRUE)
    endif()
  endif()
  cmake_pop_check_state()

endif()

if(CURSES_USE_NCURSES)
  get_filename_component(_cursesLibDir "${CURSES_NCURSES_LIBRARY}" PATH)
  get_filename_component(_cursesParentDir "${_cursesLibDir}" PATH)

  # Use CURSES_NCURSES_INCLUDE_PATH if set, for compatibility.
  if(CURSES_NCURSES_INCLUDE_PATH)
    if (CURSES_NEED_WIDE)
      find_path(CURSES_INCLUDE_PATH
        NAMES ncursesw/ncurses.h ncursesw/curses.h ncursesw.h cursesw.h
        PATHS ${CURSES_NCURSES_INCLUDE_PATH}
        NO_DEFAULT_PATH
        )
    else()
      find_path(CURSES_INCLUDE_PATH
        NAMES ncurses/ncurses.h ncurses/curses.h ncurses.h curses.h
        PATHS ${CURSES_NCURSES_INCLUDE_PATH}
        NO_DEFAULT_PATH
        )
    endif()
  endif()

  if (CURSES_NEED_WIDE)
    set(CURSES_TINFO_LIBRARY_NAME tinfow)
    find_path(CURSES_INCLUDE_PATH
      NAMES ncursesw/ncurses.h ncursesw/curses.h ncursesw.h cursesw.h
      HINTS "${_cursesParentDir}/include"
      )
  else()
    set(CURSES_TINFO_LIBRARY_NAME tinfo)
    find_path(CURSES_INCLUDE_PATH
      NAMES ncurses/ncurses.h ncurses/curses.h ncurses.h curses.h
      HINTS "${_cursesParentDir}/include"
      )
  endif()

  # Previous versions of FindCurses provided these values.
  if(NOT DEFINED CURSES_LIBRARY)
    set(CURSES_LIBRARY "${CURSES_NCURSES_LIBRARY}")
  endif()

  CHECK_LIBRARY_EXISTS("${CURSES_NCURSES_LIBRARY}"
    cbreak "" CURSES_NCURSES_HAS_CBREAK)
  CHECK_LIBRARY_EXISTS("${CURSES_NCURSES_LIBRARY}"
    nodelay "" CURSES_NCURSES_HAS_NODELAY)
  if(NOT CURSES_NCURSES_HAS_CBREAK OR NOT CURSES_NCURSES_HAS_NODELAY)
    find_library(CURSES_EXTRA_LIBRARY "${CURSES_TINFO_LIBRARY_NAME}" HINTS "${_cursesLibDir}")
    find_library(CURSES_EXTRA_LIBRARY "${CURSES_TINFO_LIBRARY_NAME}" )

    mark_as_advanced(
      CURSES_EXTRA_LIBRARY
      )
  endif()
else()
  get_filename_component(_cursesLibDir "${CURSES_CURSES_LIBRARY}" PATH)
  get_filename_component(_cursesParentDir "${_cursesLibDir}" PATH)

  #We can't find anything with CURSES_NEED_WIDE because we know
  #only about ncursesw unicode curses version
  if(NOT CURSES_NEED_WIDE)
    find_path(CURSES_INCLUDE_PATH
      NAMES curses.h
      HINTS "${_cursesParentDir}/include"
      )
  endif()

  # Previous versions of FindCurses provided these values.
  if(NOT DEFINED CURSES_CURSES_H_PATH)
    set(CURSES_CURSES_H_PATH "${CURSES_INCLUDE_PATH}")
  endif()
  if(NOT DEFINED CURSES_LIBRARY)
    set(CURSES_LIBRARY "${CURSES_CURSES_LIBRARY}")
  endif()
endif()

# Report whether each possible header name exists in the include directory.
if(NOT DEFINED CURSES_HAVE_NCURSES_NCURSES_H)
  if(CURSES_NEED_WIDE)
    if(EXISTS "${CURSES_INCLUDE_PATH}/ncursesw/ncurses.h")
      set(CURSES_HAVE_NCURSES_NCURSES_H "${CURSES_INCLUDE_PATH}/ncursesw/ncurses.h")
    endif()
  elseif(EXISTS "${CURSES_INCLUDE_PATH}/ncurses/ncurses.h")
    set(CURSES_HAVE_NCURSES_NCURSES_H "${CURSES_INCLUDE_PATH}/ncurses/ncurses.h")
  endif()
  if(NOT DEFINED CURSES_HAVE_NCURSES_NCURSES_H)
    set(CURSES_HAVE_NCURSES_NCURSES_H "CURSES_HAVE_NCURSES_NCURSES_H-NOTFOUND")
  endif()
endif()
if(NOT DEFINED CURSES_HAVE_NCURSES_CURSES_H)
  if(CURSES_NEED_WIDE)
    if(EXISTS "${CURSES_INCLUDE_PATH}/ncursesw/curses.h")
      set(CURSES_HAVE_NCURSES_CURSES_H "${CURSES_INCLUDE_PATH}/ncursesw/curses.h")
    endif()
  elseif(EXISTS "${CURSES_INCLUDE_PATH}/ncurses/curses.h")
    set(CURSES_HAVE_NCURSES_CURSES_H "${CURSES_INCLUDE_PATH}/ncurses/curses.h")
  endif()
  if(NOT DEFINED CURSES_HAVE_NCURSES_CURSES_H)
    set(CURSES_HAVE_NCURSES_CURSES_H "CURSES_HAVE_NCURSES_CURSES_H-NOTFOUND")
  endif()
endif()
if(NOT CURSES_NEED_WIDE)
  #ncursesw can't be found for this paths
  if(NOT DEFINED CURSES_HAVE_NCURSES_H)
    if(EXISTS "${CURSES_INCLUDE_PATH}/ncurses.h")
      set(CURSES_HAVE_NCURSES_H "${CURSES_INCLUDE_PATH}/ncurses.h")
    else()
      set(CURSES_HAVE_NCURSES_H "CURSES_HAVE_NCURSES_H-NOTFOUND")
    endif()
  endif()
  if(NOT DEFINED CURSES_HAVE_CURSES_H)
    if(EXISTS "${CURSES_INCLUDE_PATH}/curses.h")
      set(CURSES_HAVE_CURSES_H "${CURSES_INCLUDE_PATH}/curses.h")
    else()
      set(CURSES_HAVE_CURSES_H "CURSES_HAVE_CURSES_H-NOTFOUND")
    endif()
  endif()
endif()

find_library(CURSES_FORM_LIBRARY "${CURSES_FORM_LIBRARY_NAME}" HINTS "${_cursesLibDir}")
find_library(CURSES_FORM_LIBRARY "${CURSES_FORM_LIBRARY_NAME}" )

# Previous versions of FindCurses provided these values.
if(NOT DEFINED FORM_LIBRARY)
  set(FORM_LIBRARY "${CURSES_FORM_LIBRARY}")
endif()

# Need to provide the *_LIBRARIES
set(CURSES_LIBRARIES ${CURSES_LIBRARY})

if(CURSES_EXTRA_LIBRARY)
  set(CURSES_LIBRARIES ${CURSES_LIBRARIES} ${CURSES_EXTRA_LIBRARY})
endif()

if(CURSES_FORM_LIBRARY)
  set(CURSES_LIBRARIES ${CURSES_LIBRARIES} ${CURSES_FORM_LIBRARY})
endif()

# Provide the *_INCLUDE_DIRS and *_CFLAGS results.
set(CURSES_INCLUDE_DIRS ${CURSES_INCLUDE_PATH})
set(CURSES_INCLUDE_DIR ${CURSES_INCLUDE_PATH}) # compatibility

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(NCURSES QUIET ${NCURSES_LIBRARY_NAME})
  set(CURSES_CFLAGS ${NCURSES_CFLAGS_OTHER})
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Curses DEFAULT_MSG
  CURSES_LIBRARY CURSES_INCLUDE_PATH)

mark_as_advanced(
  CURSES_INCLUDE_PATH
  CURSES_CURSES_LIBRARY
  CURSES_NCURSES_LIBRARY
  CURSES_FORM_LIBRARY
  )
