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
  void setConfigElement(QDomElement configRoot);
  bool readVolume();
  Sweep addSweep();
  Ray addRay();

 private:
  
  bool skipReadVolume();
  bool readVolumeAnalytic();

  Configuration *config;
  // This config is the Configuration file for the analytic TC and
  // theoretical radar properties only it is not connected to the 
  // Configuration object that runs the GUI and Analysis

  QDomElement masterRoot;
  // This is the root configuration element of the master config file
  // which controlls input to GUI and Analysis

  GriddedData *data;
  float nyqVel;
  float refGateSp;
  float velGateSp;
  int numRefGates;
  int numVelGates;
  float beamWidth;
  float scale;
  float *elevations;

  float vortexLat, vortexLon;

   enum modelType {
     Rankine,
     Rossby,
     Deformation,
     MM5
  };
  
  // A whole bunch of adjustable radar properties

};

#endif
