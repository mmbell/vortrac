# CMakeLists for top-level of vortrac

cmake_minimum_required(VERSION 2.8)

project(vortrac)

# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_BUILD_TYPE Release)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# set policy CMP0100 so that AUTOMOC finds .hh files
if(${CMAKE_VERSION} VERSION_GREATER "3.17.0") 
  cmake_policy(SET CMP0100 NEW)
endif()
  
# Where to find *Find*.cmake and *-config.cmake

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_PREFIX_PATH ${CMAKE_MODULE_PATH})

# Set RPATH so that executable finds lrose libraries without needing LD_LIBRARY_PATH

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(CMAKE_INSTALL_RPATH ${LROSE_LIB_DIR})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Use RUNPATH, not RPATH

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags")
endif()

# build location - topdir/bin

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/release/bin)

# recurse into src directory for the build

add_subdirectory(src)  

