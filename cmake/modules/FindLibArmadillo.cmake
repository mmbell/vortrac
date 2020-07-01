# Try to find libarmadillo. Once done, this will define:
#
#   LIBARMADILLO_FOUND - variable which returns the result of the search
#   LIBARMADILLO_INCLUDE_DIRS - list of include directories
#   LIBARMADILLO_LIBRARIES - options for the linker

#=============================================================================
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_package(PkgConfig)
pkg_check_modules(PC_LIBARMADILLO QUIET libarmadillo)

find_path(LIBARMADILLO_INCLUDE_DIR
	armadillo.h
	HINTS ${PC_LIBARMADILLO_INCLUDEDIR} ${PC_LIBARMADILLO_INCLUDE_DIRS}
)
find_library(LIBARMADILLO_LIBRARY
	armadillo
	HINTS ${PC_LIBARMADILLO_LIBDIR} ${PC_LIBARMADILLO_LIBRARY_DIRS}
)

set(LIBARMADILLO_INCLUDE_DIRS ${LIBARMADILLO_INCLUDE_DIR})
set(LIBARMADILLO_LIBRARIES ${LIBARMADILLO_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libarmadillo DEFAULT_MSG
	LIBARMADILLO_INCLUDE_DIR
	LIBARMADILLO_LIBRARY
)

mark_as_advanced(
	LIBARMADILLO_INCLUDE_DIR
	LIBARMADILLO_LIBRARY
	)
      
