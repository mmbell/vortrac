/*
 * panels.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/18/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef PANELS_H
#define PANELS_H

#include "AbstractPanel.h"

#include <QWidget>
#include <QDoubleSpinBox>
#include <QDomElement>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QSignalMapper>
#include <QPushButton>
#include <QVBoxLayout>
#include <QRadioButton>


/*
 * These classes created the Panels for used in the ConfigurationDialog
 * They all have the same functionality as AbstractPanel,slightly altered 
 * to account for the different input parameters for each panel.
 */

class VortexPanel:public AbstractPanel
{
  // Modifies the Vortex Section of the Configuration
 public:
  VortexPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 private:
  QLineEdit *vortexName;
  QDoubleSpinBox *latBox, *longBox;

};

class RadarPanel:public AbstractPanel
{
  // Modifies the Radar Section of the Configuration 
 public:
   RadarPanel();
   void updatePanel(const QDomElement panelElement);
   bool updateConfig();
 private:
   QLineEdit *radarName;
   QDoubleSpinBox *latBox, *longBox;
   QComboBox *radarFormat;
   QDateTimeEdit *startDateTime, *endDateTime;

};

class CappiPanel:public AbstractPanel
{ 
  // Modifies the Cappi Section of the Configuration
 public:
  CappiPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 private:
  QDoubleSpinBox *xDimBox, *yDimBox, *zDimBox, *xGridBox, *yGridBox, *zGridBox;
  QDoubleSpinBox *refMinBox, *refMaxBox, *velMinBox, *velMaxBox;
  QDoubleSpinBox *advSpeedBox, *advDirBox;
  QComboBox *intBox;
};

class CenterPanel:public AbstractPanel
{
  // Modifies the Center Section of the Configuration
 public:
  CenterPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 private:
  QComboBox *geometryBox, *closureBox, *refBox, *velBox, *critBox;
  QSpinBox *bLBox,*tLBox,*iRBox,*oRBox;
  QSpinBox *iterations, *numPointsBox;
  QDoubleSpinBox *ringBox, *influenceBox,*convergenceBox, *diameterBox;
  QVBoxLayout *main;
};

class VTDPanel:public AbstractPanel
{ 
  // Modifies the VTD Section of the Configuration
 public:
  VTDPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 private:
  QComboBox *geometryBox, *closureBox, *refBox, *velBox;
  QSpinBox *bLBox,*tLBox,*iRBox, *oRBox;
  QDoubleSpinBox *ringBox;
};

class HVVPPanel:public AbstractPanel
{ 
  // Modifies the HVVP Section of the Configuration
 public:
  HVVPPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
};

class PressurePanel:public AbstractPanel
{ 
  // Modifies the Pressure Section of the Configuration
 public:
  PressurePanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 private:
};

class GraphicsPanel:public AbstractPanel
{
  // Modifies the Graphics Section of the Configuration
 public:
  GraphicsPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 private:
  QDoubleSpinBox *pMaxBox, *pMinBox, *rmwMaxBox, *rmwMinBox;
  QGroupBox *graphParameters;
  QRadioButton *manualOverride;
  QDateTimeEdit *beginTime, *endTime;
};

class QCPanel:public AbstractPanel
{
  // Modifies the QC Section of the Configuration
 public:
  QCPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 private:
  QRadioButton *vad, *user, *known;
  QSpinBox *bbSegmentSize, *maxFoldCount, *vadLevels, *numCoefficients;
  QDoubleSpinBox *velocityThreshold, *spectralThreshold, *windSpeed;
  QDoubleSpinBox *windDirection;
  
};

#endif
