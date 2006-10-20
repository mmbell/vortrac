/*
 *  AnalyticRadar.h 
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 02/07/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef ANALYTICRADAR_H
#define ANALYTICRADAR_H

#include "RadarData.h"
#include "Config/Configuration.h"
#include "DataObjects/GriddedData.h"
#include "DataObjects/GriddedFactory.h"
#include <QDomElement>

class AnalyticRadar : public RadarData
{

 public:
  AnalyticRadar(const QString& radarname = QString(), 
		float lat = 0, float lon = 0, 
	const QString& configFile = QString("vortrac_defaultAnalyticTC.xml"));
  
  ~AnalyticRadar();
  void setConfigElement(Configuration *newConfig);
  bool readVolume();

 private:

  Sweep addSweep();
  // Adds a sweep of sampled data to the analytic radar volume

  Ray addRay();
  // Adds a ray of sampled data to the analytic radar volume
  
  bool skipReadVolume();
  // Called if the analytic test data does not need to be sampled
  // This normally means that the data analysis starts at cappi
  // and continues from there without testing cappi or radarQC

  bool readVolumeAnalytic();
  // Called in order to sample a griddedData object with an analytic radar
  // to create an analytic radar volume which moves through the entire 
  // data analysis process

  Configuration *config;
  // This config is the Configuration file for the analytic TC and
  // theoretical radar properties only it is not connected to the 
  // Configuration object that runs the GUI and Analysis

  Configuration *mainConfig;
  // This is the root configuration element of the master config file
  // which controlls input to GUI and Analysis

  GriddedData *data;
  // Contains the analytic data

  float velNull;

  float nyqVel;
  float refGateSp;
  float velGateSp;
  int numRefGates;
  int numVelGates;
  float beamWidth;
  float noiseScale;
  int noisyGates;
  // Analytic radar parameter, which are read from the configuration

  float *elevations;
  // elevations contains the sweep angles used

  float vortexLat, vortexLon;

   enum modelType {
     Rankine,
     Rossby,
     Deformation,
     MM5
  };

   // modelType is all the possible types of analytic data that can
   // be constructed, each one requires different input parameter in 
   // the configuration and not all of them are fully functional yet

};

#endif
