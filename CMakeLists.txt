# CMakeLists for top-level of vortrac

cmake_minimum_required(VERSION 2.8)

project(vortrac)

# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_BUILD_TYPE Release)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# Where to find *Find*.cmake and *-config.cmake

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_PREFIX_PATH ${CMAKE_MODULE_PATH})

# required packages

find_package(Lrose COMPONENTS Ncxx Radx tdrp netcdf REQUIRED)	
find_package(LibZip REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Armadillo REQUIRED)
find_package(Qt5 COMPONENTS Core Gui Widgets Xml Network REQUIRED PATHS /usr NO_DEFAULT_PATH)

# Finding Qt on mac OSX

if(APPLE)
  find_path(Qt5_DIR NAMES Qt5Config.cmake qt5-config.cmake HINTS /usr/local/Cellar/qt/*/lib/cmake/Qt5 NO_DEFAULT_PATH)
endif(APPLE)

# set automatic creation of MOC files

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set RPATH so that executable finds lrose libraries without needing LD_LIBRARY_PATH

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(CMAKE_INSTALL_RPATH ${LROSE_LIB_DIR})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Use RUNPATH, not RPATH

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags")
endif()

# add compile definitions

add_definitions(-Wall -std=c++11 -fPIC)

# build location - topdir/bin

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# recurse into src directory for the build

add_subdirectory(src)  

