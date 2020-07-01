# CMakeLists for top-level of vortrac

cmake_minimum_required (VERSION 2.8)

project (vortrac)

set (CMAKE_BUILD_TYPE Release)

set (CMAKE_CXX_STANDARD 11)
add_definitions ( -Wall -std=c++11 -fPIC )

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

set (CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Where to find *Find*.cmake and *-config.cmake

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set (CMAKE_PREFIX_PATH ${CMAKE_MODULE_PATH})

# set (CMAKE_FIND_DEBUG_MODE True)

if (APPLE)
  find_path (Qt5_DIR NAMES Qt5Config.cmake qt5-config.cmake HINTS /usr/local/Cellar/qt/*/lib/cmake/Qt5 NO_DEFAULT_PATH)
endif (APPLE)

# required packages

find_package (Lrose COMPONENTS Ncxx Radx tdrp netcdf REQUIRED)	
find_package (LibZip REQUIRED)
find_package (BZip2 REQUIRED)
find_package (Armadillo REQUIRED)
find_package (Qt5 COMPONENTS Core Gui Widgets Xml Network REQUIRED PATHS /usr NO_DEFAULT_PATH)

# set automatic creation of MOC files

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set RPATH so that executable finds lrose libraries without needing LD_LIBRARY_PATH

set ( CMAKE_SKIP_BUILD_RPATH FALSE )
set ( CMAKE_BUILD_WITH_INSTALL_RPATH TRUE )

set ( CMAKE_INSTALL_RPATH ${LROSE_LIB_DIR} )
set ( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )

# Use RUNPATH, not RPATH

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set ( CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags" )
endif()

# build location - topdir/bin

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# recurse into src directory for the build

add_subdirectory (src)  

