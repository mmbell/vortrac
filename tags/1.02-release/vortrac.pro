######################################################################
# Automatically generated by qmake (2.00a) Thu Jul 21 17:39:59 2005
######################################################################

TEMPLATE = app
DEPENDPATH += . Config Grids GUI IO Radar
INCLUDEPATH += . GUI Config IO Radar

# Input
HEADERS += Threads/AnalysisThread.h \
           Threads/PollThread.h \
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
           GUI/TestGraph.h \
           GUI/AbstractPanel.h \ 
           GUI/panels.h \
           GUI/DiagnosticPanel.h \
           GUI/RadarListDialog.h \
           GUI/StopLight.h \ 
           GUI/CappiDisplay.h \
           GUI/StormSignal.h \
           HVVP/Hvvp.h \
           IO/Message.h \
           IO/Log.h \
           Radar/RadarFactory.h \
           Radar/LevelII.h \
           Radar/NcdcLevelII.h \
           Radar/LdmLevelII.h \
           Radar/AnalyticRadar.h \
           Radar/nexh.h \
           Radar/RadarQC.h \
           Radar/RadarData.h \
           Radar/Ray.h \
           Radar/Sweep.h \
           VTD/GBVTD.h \
           Math/Matrix.h \
           ChooseCenter.h \
           Pressure/PressureData.h \
           Pressure/PressureList.h \
           Pressure/PressureFactory.h \
           Pressure/HWind.h \
           Pressure/AWIPS.h
SOURCES += main.cpp \
           Threads/AnalysisThread.cpp \
           Threads/PollThread.cpp \
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
           GUI/TestGraph.cpp \
           GUI/AbstractPanel.cpp \
           GUI/panels.cpp \
           GUI/DiagnosticPanel.cpp \
           GUI/RadarListDialog.cpp \
           GUI/StopLight.cpp \
           GUI/CappiDisplay.cpp \
           GUI/StormSignal.cpp \
           HVVP/Hvvp.cpp \
           IO/Message.cpp \
           IO/Log.cpp \
           Radar/RadarFactory.cpp \
           Radar/LevelII.cpp \
           Radar/NcdcLevelII.cpp \
           Radar/LdmLevelII.cpp \
           Radar/AnalyticRadar.cpp\
           Radar/RadarQC.cpp \
           Radar/RadarData.cpp \
           Radar/Ray.cpp \
           Radar/Sweep.cpp \
           VTD/GBVTD.cpp \
           Math/Matrix.cpp \
           ChooseCenter.cpp \
           Pressure/PressureData.cpp \
           Pressure/PressureList.cpp \
           Pressure/PressureFactory.cpp \
           Pressure/HWind.cpp \
           Pressure/AWIPS.cpp
RESOURCES += vortrac.qrc
LIBS += -lbz2
QT += xml
CONFIG += release