######################################################################
# Dependencies:
#
#   git
#   gcc
#   gcc-c++
#   libX11-devel
#   libXext-devel
#   qt5-qtbase-devel
#   zlib-devel
#   bzip2-devel
#   hdf5-devel
#   netcdf-devel 
#   armadillo-devel
#
# Also depends on lrose-core libraries.
# Expects lrose-core to be installed in:
#   /usr/local/lrose or $HOME/lrose
#
# If lrose is in a non-standard location,
# set env LROSE_INSTALL_DIR to that location.
#
######################################################################

TEMPLATE = app

DEPENDPATH += . Config Grids GUI IO Radar

INCLUDEPATH += . GUI Config IO Radar
INCLUDEPATH += /usr/local/include $$(HOME)/lrose/include /usr/local/lrose/include

# add the following temporarily - Debian 10 and Ubuntu 19 seem to have
# a bug in qmake which points to /usr/lib instead of /usr/include
INCLUDEPATH += /usr/include/x86_64-linux-gnu/qt5
INCLUDEPATH += /usr/include/x86_64-linux-gnu/qt5/QtGui
INCLUDEPATH += /usr/include/x86_64-linux-gnu/qt5/QtXml
INCLUDEPATH += /usr/include/x86_64-linux-gnu/qt5/QtNetwork
INCLUDEPATH += /usr/include/x86_64-linux-gnu/qt5/QtCore
INCLUDEPATH += /usr/include/x86_64-linux-gnu/qt5/QtWidgets

QMAKE_LIBDIR += $$(HOME)/lrose/lib /usr/local/lrose/lib /usr/local/lib
QMAKE_LIBDIR += /usr/lib/x86_64-linux-gnu/hdf5/serial

lroseDir = $$(LROSE_INSTALL_DIR)
!isEmpty(lroseDir) {
  INCLUDEPATH += $$lroseDir/include
  QMAKE_LIBDIR += $$lroseDir/lib
}

#!macx {
#   QMAKE_LFLAGS += -Wl,-rpath,$$(NETCDF_LIB) -L/usr/local/lib
#}

QMAKE_CXXFLAGS += -std=c++0x

# Input

HEADERS += Threads/workThread.h \
           Threads/SimplexThread.h \
           Threads/VortexThread.h \
           DataObjects/VortexData.h \
           DataObjects/SimplexData.h \
           DataObjects/VortexList.h \
           DataObjects/SimplexList.h \
           DataObjects/Coefficient.h \
           DataObjects/Center.h \
           Config/Configuration.h \
           DataObjects/AnalyticGrid.h \
           DataObjects/CappiGrid.h \
           DataObjects/GriddedData.h \
           DataObjects/GriddedFactory.h \
           GUI/ConfigTree.h \
           GUI/ConfigurationDialog.h \
           GUI/MainWindow.h \
           GUI/AnalysisPage.h \
           GUI/GraphFace.h \
           GUI/KeyPicture.h \
           GUI/AbstractPanel.h \
           GUI/panels.h \
           GUI/DiagnosticPanel.h \
           GUI/RadarListDialog.h \
           GUI/StopLight.h \
           GUI/CappiDisplay.h \
           GUI/StormSignal.h \
           GUI/StartDialog.h \
           NRL/Hvvp.h \
           IO/Message.h \
           IO/Log.h \
           IO/ATCF.h \
           Radar/DateChecker.h \
           Radar/RadarFactory.h \
           Radar/LevelII.h \
           Radar/NcdcLevelII.h \
           Radar/RadxGrid.h \
           Radar/LdmLevelII.h \
           Radar/RadxData.h \
           Radar/AnalyticRadar.h \
           Radar/nexh.h \
           NRL/RadarQC.h \
           Radar/RadarData.h \
           Radar/Ray.h \
           Radar/Sweep.h \
           VTD/VTD.h \
           VTD/GVTD.h \
           VTD/GBVTD.h \
           VTD/mgbvtd.h \
           VTD/VTDFactory.h \
           Math/Matrix.h \
           ChooseCenter.h \
           Pressure/PressureData.h \
           Pressure/PressureList.h \
           Pressure/PressureFactory.h \
           Pressure/HWind.h \
           Pressure/AWIPS.h \
           Pressure/MADIS.h \
           Pressure/MADISFactory.h \
           Radar/FetchRemote.h \
           Batch/DriverBatch.h \
           Batch/BatchWindow.h \
           DriverAnalysis.h

SOURCES += main.cpp \
           Threads/workThread.cpp \
           Threads/SimplexThread.cpp \
           Threads/VortexThread.cpp \
           DataObjects/VortexData.cpp \
           DataObjects/SimplexData.cpp \
           DataObjects/VortexList.cpp \
           DataObjects/SimplexList.cpp \
           DataObjects/Coefficient.cpp \
           DataObjects/Center.cpp \
           Config/Configuration.cpp \
           DataObjects/AnalyticGrid.cpp \
           DataObjects/CappiGrid.cpp \
           DataObjects/GriddedData.cpp \
           DataObjects/GriddedFactory.cpp \
           GUI/ConfigTree.cpp \
           GUI/ConfigurationDialog.cpp \
           GUI/MainWindow.cpp \
           GUI/AnalysisPage.cpp \
           GUI/GraphFace.cpp \
           GUI/KeyPicture.cpp \
           GUI/AbstractPanel.cpp \
           GUI/panels.cpp \
           GUI/DiagnosticPanel.cpp \
           GUI/RadarListDialog.cpp \
           GUI/StopLight.cpp \
           GUI/CappiDisplay.cpp \
           GUI/StormSignal.cpp \
           GUI/StartDialog.cpp \
           NRL/Hvvp.cpp \
           IO/Message.cpp \
           IO/Log.cpp \
           IO/ATCF.cpp \
           Radar/DateChecker.cpp \
           Radar/RadarFactory.cpp \
           Radar/LevelII.cpp \
           Radar/NcdcLevelII.cpp \
           Radar/RadxGrid.cpp \
           Radar/LdmLevelII.cpp \
           Radar/RadxData.cpp \
           Radar/AnalyticRadar.cpp\
           NRL/RadarQC.cpp \
           Radar/RadarData.cpp \
           Radar/Ray.cpp \
           Radar/Sweep.cpp \
           VTD/VTD.cpp \
           VTD/GVTD.cpp \
           VTD/GBVTD.cpp \
           VTD/mgbvtd.cpp \
           VTD/VTDFactory.cpp \
           Math/Matrix.cpp \
           ChooseCenter.cpp \
           Pressure/PressureData.cpp \
           Pressure/PressureList.cpp \
           Pressure/PressureFactory.cpp \
           Pressure/HWind.cpp \
           Pressure/AWIPS.cpp \
           Pressure/MADIS.cpp \
           Pressure/MADISFactory.cpp \
           Radar/FetchRemote.cpp \
           Batch/DriverBatch.cpp \
           Batch/BatchWindow.cpp \
           DriverAnalysis.cpp

RESOURCES += vortrac.qrc
LIBS += -lRadx -lNcxx -lnetcdf -lhdf5_cpp -lhdf5 -larmadillo -lz -lbz2
QT += xml network widgets

# CONFIG += debug
#CONFIG -= app_bundle
