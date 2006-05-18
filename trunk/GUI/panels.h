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
#include "Configuration.h"

#include <QWidget>
#include <QDoubleSpinBox>
#include <QDomElement>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QSignalMapper>
#include <QPushButton>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QPointF>

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
  // Widget used to receive/display hurricane name
  
  QDoubleSpinBox *latBox, *longBox;
  // Widgets used to recieve/display hurricane latitude and longitude

};

class RadarPanel:public AbstractPanel
{
  // Modifies the Radar Section of the Configuration 
 public:
   RadarPanel();
   void updatePanel(const QDomElement panelElement);
   bool updateConfig();

 private:
   /* 
    *  radarName, latBox and longBox are used in this panel but
    *  declared in AbstractPanel to get them to interact with 
    *  signal slot operations.
    *  This panel also uses dir and the browse button which are declared
    *  in AbstractPanel, to obtain and store the location of incoming 
    *  radar data, or to obtain the xml input when working with analytic
    *  models.
    */

   QComboBox *radarFormat;
   // Allows selection of the input format for the radar data

   QHash<QString, QString> *radarFormatOptions;
   // The radarFormatOptions hash is used to relate combobox entries 
   // to the correct element in the xml file.

   QDateTimeEdit *startDateTime, *endDateTime;
   // Widgets used to display the start and end date and time for running
   // the vortrac algorithm

  
};

class CappiPanel:public AbstractPanel
{ 
  // Modifies the Cappi Section of the Configuration
 public:
  CappiPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();

 private:
  /*
   * This panel uses the dir and browse members declared in AbstractPanel
   * for the location of the cappi output directory.
   */

  QDoubleSpinBox *xDimBox, *yDimBox, *zDimBox, *xGridBox, *yGridBox, *zGridBox;
  // These boxes take in the lengths of the 3-D grid for mapping radar data 
  // onto (xDimBox is for length in x dimension etc). The members labeled x, 
  // y and x GridBoxes are for collecting and editing the distance between
  // data points in the cappi grid in each dimension.

  QDoubleSpinBox *advSpeedBox, *advDirBox;
  // Determines the speed and direction of advection flow

  QComboBox *intBox;
  // Allows the user to select an interpolation method from those listed
  // in the interpolationMethod hash. This set the interpolation method 
  // to be used in creating the cappi grid.

  QHash<QString, QString> *interpolationMethod;
  // The interpolationMethod hash is used to relate combobox entries to 
  // interpolation key words

};

class CenterPanel:public AbstractPanel
{
  // Modifies the Center Section of the Configuration
 public:
  CenterPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();

 private:
  /*
   * This panel uses the dir and browse members declared in AbstractPanel
   * for the location of the center output directory.
   * This panel also uses the dataGap members declared in AbstractPanel
   * for automatically updating the number of input boxes available.
   */
  QComboBox *geometryBox, *closureBox, *refBox, *velBox, *critBox;
  // displays the options for each of these control parameters
  // each ComboBox has a cooresponding hash of possible parameters
  QHash<QString, QString> *geometryOptions;
  QHash<QString, QString> *closureOptions;
  QHash<QString, QString> *reflectivityOptions;
  QHash<QString, QString> *velocityOptions;
  QHash<QString, QString> *criteriaOptions;
  
  QSpinBox *bLBox,*tLBox,*iRBox,*oRBox;
  // bLBox controls the lowest search level of the cappi
  // tLBox controls the highest search level of the cappi
  // iRBox controls the inner search radius
  // oRBox controls the outer search radius

  QSpinBox *iterations, *numPointsBox;
  // these boxes control/display the maximum number of iteration that the 
  // centerfinder should attempt, and the number of initial guesses to be 
  // attempting in finding the center

  QDoubleSpinBox *ringBox, *influenceBox,*convergenceBox, *diameterBox;
  // ringBox is associated with the width of the search rings to be used
  // influenceBox is associated with the radius of influence to be used
  // convergenceBox is associated with the required convergence
  // diameterBox is associated with the width of the zone to be searched
  //  for the vortrex center
};

class VTDPanel:public AbstractPanel
{ 
  // Modifies the VTD Section of the Configuration
 public:
  VTDPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 
 private:
  /*
   * This panel uses the dir and browse members declared in AbstractPanel
   * for the location of the vtd  output directory.
   * This panel also uses the dataGap members declared in AbstractPanel
   * for automatically updating the number of input boxes available.
   */

  QComboBox *geometryBox, *closureBox, *refBox, *velBox;
  // displays the options for each of these control parameters
  // each ComboBox has a cooresponding hash of possible parameters
  QHash<QString, QString> *geometryOptions;
  QHash<QString, QString> *closureOptions;
  QHash<QString, QString> *reflectivityOptions;
  QHash<QString, QString> *velocityOptions;
  QHash<QString, QString> *criteriaOptions;

  QSpinBox *bLBox, *tLBox, *iRBox, *oRBox;
  // bLBox controls the lowest search level of the cappi
  // tLBox controls the highest search level of the cappi
  // iRBox controls the inner search radius
  // oRBox controls the outer search radius

  QDoubleSpinBox *ringBox;
  // controls the width of the search rings
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
  /*
   * This panel uses the dir and browse members declared in AbstractPanel
   * for the location of the directory containing pressure data.
   */
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
  QDateTimeEdit *beginTime, *endTime;
  /*
   * These are used to manipulate the visible limits of the pressure &
   *   rmw display graph
   * pMaxBox: maximum pressure displayed on graph
   * pMinBox: minimum pressure displayed on graph
   * rmwMaxBox: maximum rmw displayed on graph
   * rmwMinBox: minimum rmw displayed on graph
   * beginTime: controls lower time limit on graph
   * endTime: controls upper time limit displayed on graph
   */

  QGroupBox *graphParameters;
  // used to distinguish weither graph limits are adjusted automatically or
  //   manually.  

};

class QCPanel:public AbstractPanel
{
  // Modifies the QC Section of the Configuration
 public:
  QCPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 private:
  /*
   * This panel uses the dir and browse members declared in AbstractPanel
   * for the location of the AWIPS data directory.
   */
  QRadioButton *vad, *user, *known;
  /*
   * These buttons are all mutually exclusive choices which decide how the 
   * environmental winds should be determined.
   * vad: uses the VAD algorithm
   * user: uses user entered winds for environmental winds
   * known: uses AWIPS data when available
   */

  QSpinBox *bbSegmentSize, *maxFoldCount, *vadLevels, *numCoefficients;
 
  /*
   * bbSegmentSize handles the number of gates that should be used for 
   *   averaging in the Bargain & Brown dealiasing algorithm
   * 
   * maxFoldCount sets the maximum number of velocity folds before they are
   *   reported as an error
   * 
   * valLevels indicates the number of levels the VAD algorithm should generate
   *   (used only when the VAD method is used)
   * 
   * numCoefficients indicates the number of coefficients used for the VAD
   *   algorithm fit (used only when the VAD method is used)
   */

  QDoubleSpinBox *velocityMinimum, *velocityMaximum, *spectralThreshold;
  QDoubleSpinBox *reflectivityMinimum, *reflectivityMaximum, *windSpeed;
  QDoubleSpinBox *windDirection;
  /*
   * velocityMinimum adjusts the minimum absolute value of velocities that 
   *   will be used for analysis (attempts to remove ground clutter)
   * velocityMaximum adjusts the maximum absolute value of velocities that 
   *   will be used for analysis (attempts to remove erroneous data)
   * reflectivityMinimum adjusts the minimum value of reflectivity that 
   *   will be used for analysis (attempts to remove erroneous data)
   * reflectivityMaximum adjusts the maximum value of reflectivity that 
   *   will be used for analysis (attempts to remove erroneous data)
   * spectralThreshold adjusts the minimum spectral width required for a 
   *   velocities that will be used for analysis
   * windSpeed is used to adjust the user entered environmental wind speed
   *   (used only when user entered environmental winds are used)
   * windDirection is used to adjust the user entered environmental wind 
   *   direction (used only when user entered environmental winds are used)
   */


  
};

#endif
