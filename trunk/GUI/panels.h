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
  ~VortexPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();

 private:
  QLineEdit *vortexName;   
  // Widget used to receive/display hurricane name
  
  QDoubleSpinBox *latBox, *longBox, *directionBox, *speedBox;
  /*
   * latBox & lonBox are used to adjust the starting latitude and longitude
   *   of the storm of interest. This only needs to be initialized for the 
   *   volume analyzed, all subsequent volumes use previous centers as 
   *   analysis starting points.
   *
   * directionBox & speedBox are used to set the initial direction of motion
   *   and the initial speed of the storm. Supply this information is not
   *   required. If this information is supplied it will be used to 
   *   extrapolate a starting point if the program is initialized when the
   *   storm is still out of radar range.
   *
   */

};

class RadarPanel:public AbstractPanel
{
  // Modifies the Radar Section of the Configuration 
 public:
   RadarPanel();
   ~RadarPanel();
   void updatePanel(const QDomElement panelElement);
   bool updateConfig();
   bool checkDates();

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
  ~CappiPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
  bool setDefaultDirectory(QDir* newDir);

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

  QDoubleSpinBox *advUWindBox, *advVWindBox;
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
  ~CenterPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
  bool setDefaultDirectory(QDir* newDir);

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

class ChooseCenterPanel:public AbstractPanel
{
 public:
  ChooseCenterPanel();
  ~ChooseCenterPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
  bool setDefaultDirectory(QDir* newDir);
  bool checkDates();

 private:
  /*
   * This panel uses the dir and browse members declared in AbstractPanel
   * for the location of the center output directory.
   * This panel also uses the dataGap members declared in AbstractPanel
   * for automatically updating the number of input boxes available.
   */
  
  QDateTimeEdit *startDateTime, *endDateTime;
  /*
   * startDateTime & endDateTime adjust the parameters that control upper 
   *  and lower limits of volumes with valid dates to be used in the fit
   *  
   */

  QDoubleSpinBox *windWeightBox, *stdDevWeightBox, *ptsWeightBox;
  /* 
   * windWeightBox contains the relative weight a volume's maximum wind 
   *   will be given in determining the curve fit
   *
   * stdDevWeightBox contains the relative weight a volume's standard 
   *   deviation should be given when determing the curve fit
   *
   * ptsWeightBox contains the relative weight a volume's number of converging 
   *   points should be given when determing the curve fit
   *
   * The sum of these three values should add up to one
   *
   */
  
  QDoubleSpinBox *positionWeightBox, *rmwWeightBox, *velWeightBox;
  /* 
   * positionWeightBox contains the relative weight a center's position 
   *   will be given in determining which center best fits the curve
   *
   * rmwWeightBox contains the relative weight a center's radius of 
   *   maximum wind should be given when determining which center best fits 
   *   the curve
   *
   * velWeightBox contains the relative weight a center's maximum velocity  
   *   should be given when determining which center best fits the curve
   *
   * The sum of these three values should add up to one
   *
   */

  QRadioButton *fTest95Button, *fTest99Button;
  /*
   * fTest95Button and fTest99Button determine what agreement will be required 
   *   for fTesting results. If fTest99Button is down then the results will 
   *   be thresholded at the 99th percentile. If fTest95Button is down then
   *   agreement within the 95th percentile will be used.
   *
   */

  QSpinBox *minVolumes;
  /*
   * minVolumes contains the valuse of the minimum number of volumes required
   *   before fitting should be used to determine the best center for the 
   *   analysis. Prior to accumulating this number of processed volumes the
   *   mean center will be used.
   *
   */
};

class VTDPanel:public AbstractPanel
{ 
  // Modifies the VTD Section of the Configuration
 public:
  VTDPanel();
  ~VTDPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
  bool setDefaultDirectory(QDir* newDir);
 
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
  ~HVVPPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
};

class PressurePanel:public AbstractPanel
{ 
  // Modifies the Pressure Section of the Configuration
 public:
  PressurePanel();
  ~PressurePanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
  bool setDefaultDirectory(QDir* newDir);
  
 private:
  /*
   * This panel uses the dir and browse members declared in AbstractPanel
   * for the location of the directory containing pressure data.
   */
  QComboBox *pressureFormat;
  // Allows selection of the input format for the pressure data
  
  QHash<QString, QString> *pressureFormatOptions;
  // The pressureFormatOptions hash is used to relate combobox entries 
  // to the correct element in the xml file.

  QSpinBox *maxObsTime;
  // The number of minutes to threshold pressure observations

  QDoubleSpinBox *maxObsDist;
  // The maximum distance to threshold pressure observations on

  QRadioButton *maxObsDistCenter, *maxObsDistRing;
  // Determining the method for thresholding pressure observations
  // If maxObsDistCenter is used the measurement will be made from the
  //    TC center.
  // If maxObsDistRing is used the measurement will be made from the largest
  //    analysis ring.
  
};

class GraphicsPanel:public AbstractPanel
{
  // Modifies the Graphics Section of the Configuration
 public:
  GraphicsPanel();
  ~GraphicsPanel();
  void updatePanel(const QDomElement panelElement);
  bool updateConfig();
 private:
  QDoubleSpinBox *pMaxBox, *pMinBox, *rmwMaxBox, *rmwMinBox;
  QDateTimeEdit *startDateTime, *endDateTime;
  /*
   * These are used to manipulate the visible limits of the pressure &
   *   rmw display graph
   * pMaxBox: maximum pressure displayed on graph
   * pMinBox: minimum pressure displayed on graph
   * rmwMaxBox: maximum rmw displayed on graph
   * rmwMinBox: minimum rmw displayed on graph
   * startDateTime: controls lower time limit on graph
   * endDateTime: controls upper time limit displayed on graph
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
  ~QCPanel();
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
  QSpinBox *vadthr, *gvadthr;
 
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
