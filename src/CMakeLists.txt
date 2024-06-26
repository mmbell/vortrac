# CMakeLists for src-level of vortrac

project(vortrac)

# Specify header files

set(
  HEADERS
  Threads/workThread.h 
  Threads/SimplexThread.h 
  Threads/VortexThread.h 
  DataObjects/VortexData.h 
  DataObjects/SimplexData.h 
  DataObjects/VortexList.h 
  DataObjects/SimplexList.h 
  DataObjects/Coefficient.h 
  DataObjects/Center.h 
  Config/Configuration.h 
  DataObjects/AnalyticGrid.h 
  DataObjects/CappiGrid.h 
  DataObjects/GriddedData.h 
  DataObjects/GriddedFactory.h 
  GUI/ConfigTree.h 
  GUI/ConfigurationDialog.h 
  GUI/MainWindow.h 
  GUI/AnalysisPage.h 
  GUI/GraphFace.h 
  GUI/KeyPicture.h 
  GUI/AbstractPanel.h 
  GUI/panels.h 
  GUI/DiagnosticPanel.h 
  GUI/RadarListDialog.h 
  GUI/StopLight.h 
  GUI/CappiDisplay.h 
  GUI/StormSignal.h 
  GUI/StartDialog.h 
  NRL/Hvvp.h 
  IO/Message.h 
  IO/Log.h 
  IO/ATCF.h 
  Radar/DateChecker.h 
  Radar/RadarFactory.h 
  Radar/LevelII.h 
  Radar/NcdcLevelII.h 
  Radar/RadxGrid.h 
  Radar/LdmLevelII.h 
  Radar/RadxData.h 
  Radar/AnalyticRadar.h 
  Radar/nexh.h 
  NRL/RadarQC.h 
  Radar/RadarData.h 
  Radar/Ray.h 
  Radar/Sweep.h 
  VTD/VTD.h 
  VTD/GVTD.h 
  VTD/GBVTD.h 
  VTD/mgbvtd.h 
  VTD/VTDFactory.h 
  Math/Matrix.h 
  ChooseCenter.h 
  Pressure/PressureData.h 
  Pressure/PressureList.h 
  Pressure/PressureFactory.h 
  Pressure/HWind.h 
  Pressure/AWIPS.h 
  Pressure/MADIS.h 
  Pressure/MADISFactory.h 
  Radar/FetchRemote.h 
  Batch/DriverBatch.h 
  Batch/BatchWindow.h 
  DriverAnalysis.h
)

# Specify source files

set(
  SOURCES
  main.cpp 
  Threads/workThread.cpp 
  Threads/SimplexThread.cpp 
  Threads/VortexThread.cpp 
  DataObjects/VortexData.cpp 
  DataObjects/SimplexData.cpp 
  DataObjects/VortexList.cpp 
  DataObjects/SimplexList.cpp 
  DataObjects/Coefficient.cpp 
  DataObjects/Center.cpp 
  Config/Configuration.cpp 
  DataObjects/AnalyticGrid.cpp 
  DataObjects/CappiGrid.cpp 
  DataObjects/GriddedData.cpp 
  DataObjects/GriddedFactory.cpp 
  GUI/ConfigTree.cpp 
  GUI/ConfigurationDialog.cpp 
  GUI/MainWindow.cpp 
  GUI/AnalysisPage.cpp 
  GUI/GraphFace.cpp 
  GUI/KeyPicture.cpp 
  GUI/AbstractPanel.cpp 
  GUI/panels.cpp 
  GUI/DiagnosticPanel.cpp 
  GUI/RadarListDialog.cpp 
  GUI/StopLight.cpp 
  GUI/CappiDisplay.cpp 
  GUI/StormSignal.cpp 
  GUI/StartDialog.cpp 
  NRL/Hvvp.cpp 
  IO/Message.cpp 
  IO/Log.cpp 
  IO/ATCF.cpp 
  Radar/DateChecker.cpp 
  Radar/RadarFactory.cpp 
  Radar/LevelII.cpp 
  Radar/NcdcLevelII.cpp 
  Radar/RadxGrid.cpp 
  Radar/LdmLevelII.cpp 
  Radar/RadxData.cpp 
  Radar/AnalyticRadar.cpp
  NRL/RadarQC.cpp 
  Radar/RadarData.cpp 
  Radar/Ray.cpp 
  Radar/Sweep.cpp 
  VTD/VTD.cpp 
  VTD/GVTD.cpp 
  VTD/GBVTD.cpp 
  VTD/mgbvtd.cpp 
  VTD/VTDFactory.cpp 
  Math/Matrix.cpp 
  ChooseCenter.cpp 
  Pressure/PressureData.cpp 
  Pressure/PressureList.cpp 
  Pressure/PressureFactory.cpp 
  Pressure/HWind.cpp 
  Pressure/AWIPS.cpp 
  Pressure/MADIS.cpp 
  Pressure/MADISFactory.cpp 
  Radar/FetchRemote.cpp 
  Batch/DriverBatch.cpp 
  Batch/BatchWindow.cpp 
  DriverAnalysis.cpp
)

set(
  MOC_SOURCES
  moc_ATCF.cpp
  moc_AbstractPanel.cpp
  moc_AnalysisPage.cpp
  moc_BatchWindow.cpp
  moc_CappiDisplay.cpp
  moc_ConfigTree.cpp
  moc_Configuration.cpp
  moc_ConfigurationDialog.cpp
  moc_DiagnosticPanel.cpp
  moc_DriverAnalysis.cpp
  moc_DriverBatch.cpp
  moc_FetchRemote.cpp
  moc_GraphFace.cpp
  moc_Hvvp.cpp
  moc_KeyPicture.cpp
  moc_Log.cpp
  moc_MADISFactory.cpp
  moc_MainWindow.cpp
  moc_PressureFactory.cpp
  moc_RadarFactory.cpp
  moc_RadarListDialog.cpp
  moc_RadarQC.cpp
  moc_SimplexThread.cpp
  moc_StartDialog.cpp
  moc_StopLight.cpp
  moc_StormSignal.cpp
  moc_VortexThread.cpp
)

# Finding pkg-config

find_package(PkgConfig REQUIRED)

# Finding Qt on mac OSX

if(APPLE)
  find_path(Qt5_DIR NAMES Qt5Config.cmake qt5-config.cmake HINTS /usr/local/Cellar/qt@5/*/lib/cmake/Qt5 /opt/homebrew/Cellar/qt@5/*/lib/cmake/Qt5 NO_DEFAULT_PATH)
endif(APPLE)

# required packages

#  X11, hdf5 and netcdf

find_package (X11)
if (DEFINED X11_X11_LIB)
  get_filename_component(X11_LIB_DIR ${X11_X11_LIB} DIRECTORY)
endif()
find_package (HDF5)
find_package (NETCDF)
if (DEFINED HDF5_hdf5_LIBRARY_RELEASE)
  get_filename_component(HDF5_INSTALL_PREFIX ${HDF5_hdf5_LIBRARY_RELEASE} DIRECTORY)
endif()
message("X11_X11_INCLUDE_PATH: ${X11_X11_INCLUDE_PATH}")
message("X11_LIB_DIR: ${X11_LIB_DIR}")
message("netCDF_INSTALL_PREFIX: ${netCDF_INSTALL_PREFIX}")
message("HDF5_INSTALL_PREFIX: ${HDF5_INSTALL_PREFIX}")
message("HDF5_C_INCLUDE_DIR: ${HDF5_C_INCLUDE_DIR}")

# required LROSE, ARMADILLO

find_package(LROSE REQUIRED)	
find_package(Armadillo REQUIRED)

# Qt

# Finding Qt

find_package (Qt5 COMPONENTS Core Gui Widgets Xml Network QUIET)
if (NOT Qt5_FOUND)
  find_package (Qt6 COMPONENTS Core Gui Widgets Xml Network QUIET)
endif()

if (Qt5_FOUND)
  message(STATUS "Found Qt5: ${Qt5_VERSION}")
elseif (Qt6_FOUND)
  message(STATUS "Found Qt6: ${Qt6_VERSION}")
else ()
  message(FATAL_ERROR, "Qt not found.")
endif(Qt5_FOUND)

if(APPLE)
  if (Qt5_FOUND)
    find_path(Qt5_DIR NAMES Qt5Config.cmake qt5-config.cmake HINTS /usr/local/Cellar/qt/*/lib/cmake/Qt5 /opt/homebrew/Cellar/qt/*/lib/cmake/Qt5 $ENV{HOME}/homebrew/Cellar/qt/*/lib/cmake/Qt5 NO_DEFAULT_PATH)
  elseif (Qt6_FOUND)
    find_path(Qt6_DIR NAMES Qt6Config.cmake qt6-config.cmake HINTS /usr/local/Cellar/qt/*/lib/cmake/Qt6 /opt/homebrew/Cellar/qt/*/lib/cmake/Qt6 $ENV{HOME}/homebrew/Cellar/qt/*/lib/cmake/Qt6 NO_DEFAULT_PATH)
  endif(Qt5_FOUND)
endif(APPLE)

if (Qt5_FOUND)
#QT5
  find_package (Qt5 COMPONENTS Widgets Network Qml REQUIRED PATHS /usr /usr/local/opt/qt NO_DEFAULT_PATH)
  pkg_search_module(Qt5Core REQUIRED)
  pkg_search_module(Qt5Gui REQUIRED)
  pkg_search_module(Qt5Widgets REQUIRED)
  pkg_search_module(Qt5Network REQUIRED)
  pkg_search_module(Qt5Qml REQUIRED)
else()
#QT6
  find_package (Qt6 COMPONENTS Widgets Network Qml REQUIRED PATHS /usr /usr/local/opt/qt NO_DEFAULT_PATH)
  pkg_search_module(Qt6Core REQUIRED)
  pkg_search_module(Qt6Gui REQUIRED)
  pkg_search_module(Qt6Widgets REQUIRED)
  pkg_search_module(Qt6Network REQUIRED)
  pkg_search_module(Qt6Qml REQUIRED)
endif(Qt5_FOUND)

# set automatic creation of MOC files

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# compile definitions

if (Qt5_FOUND)
#QT5
  add_definitions(-std=c++11)
else()
#QT6
  add_definitions(-std=c++17)
endif(Qt5_FOUND)

# Set include dirs

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${LROSE_PREFIX}/include)
include_directories(${LROSE_INSTALL_DIR}/include)

if (Qt5_FOUND)
  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  include_directories(${Qt5Gui_INCLUDE_DIRS})
  include_directories(${Qt5Xml_INCLUDE_DIRS})
  include_directories(${Qt5Network_INCLUDE_DIRS})
  include_directories(${Qt5Core_INCLUDE_DIRS})
else()
#QT6
  include_directories(${Qt6Widgets_INCLUDE_DIRS})
  include_directories(${Qt6Gui_INCLUDE_DIRS})
  include_directories(${Qt6Xml_INCLUDE_DIRS})
  include_directories(${Qt6Network_INCLUDE_DIRS})
  include_directories(${Qt6Core_INCLUDE_DIRS})
endif(Qt5_FOUND)
  
if (DEFINED X11_X11_INCLUDE_PATH)
  include_directories (${X11_X11_INCLUDE_PATH})
endif()
if (DEFINED netCDF_INSTALL_PREFIX)
  include_directories (${netCDF_INSTALL_PREFIX}/include)
endif()
if (DEFINED HDF5_C_INCLUDE_DIR)
  include_directories (${HDF5_C_INCLUDE_DIR})
endif()
if(IS_DIRECTORY /usr/include/hdf5/serial)
  include_directories (/usr/include/hdf5/serial)
endif()
if(IS_DIRECTORY /usr/local/include)
  include_directories (/usr/local/include)
endif()

# set link dirs

link_directories($ENV{HOME}/lrose/lib)
link_directories(${LROSE_PREFIX}/lib)
link_directories(${LROSE_INSTALL_DIR}/lib)
link_directories(/usr/local/lib)
link_directories(/usr/lib/x86_64-linux-gnu/hdf5/serial)
if (DEFINED X11_LIB_DIR)
  link_directories (${X11_LIB_DIR})
endif()
if (DEFINED netCDF_INSTALL_PREFIX)
  link_directories (${netCDF_INSTALL_PREFIX}/lib)
endif()
if (DEFINED HDF5_INSTALL_PREFIX)
  link_directories (${HDF5_INSTALL_PREFIX}/lib)
endif()
if (DEFINED HDF5_LIBRARY_DIRS)
  link_directories(${HDF5_LIBRARY_DIRS})
endif()
# add serial, for odd Debian hdf5 install
if(IS_DIRECTORY /usr/lib/x86_64-linux-gnu/hdf5/serial)
  link_directories(/usr/lib/x86_64-linux-gnu/hdf5/serial)
endif()
if(IS_DIRECTORY /usr/local/lib)
  link_directories (/usr/local/lib)
endif()

# we are building vortrac

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

# link

target_link_libraries(${PROJECT_NAME} ${LROSE_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${LIBZIP_LIBRARIES} bz2)
target_link_libraries(${PROJECT_NAME} ${LIBARMADILLO_LIBRARIES})

if (Qt5_FOUND)
  target_link_libraries(${PROJECT_NAME} ${Qt5Widgets_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Qt5Gui_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Qt5Xml_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Qt5Network_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Qt5Core_LIBRARIES})
else()
#QT6
  target_link_libraries(${PROJECT_NAME} ${Qt6Widgets_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Qt6Gui_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Qt6Xml_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Qt6Network_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Qt6Core_LIBRARIES})
endif(Qt5_FOUND)

target_link_libraries(${PROJECT_NAME} armadillo)

# install

set(INSTALL_PREFIX $ENV{VORTRAC_INSTALL_DIR})

if(NOT INSTALL_PREFIX)
  set(INSTALL_PREFIX ${LROSE_PREFIX})
endif(NOT INSTALL_PREFIX)

install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_PREFIX}/bin)

