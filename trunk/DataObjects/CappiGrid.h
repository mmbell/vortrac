/*
 *  CappiGrid.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/29/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef CAPPIGRID_H
#define CAPPIGRID_H

#include <QDomElement>
#include <QFile>
#include "Radar/RadarData.h"
#include "DataObjects/GriddedData.h"

class CappiGrid : public GriddedData
{
 
 public:
  CappiGrid();
  ~CappiGrid();
  void gridRadarData(RadarData *radarData, QDomElement cappiConfig,
		float *vortexLat, float *vortexLon);
  void BarnesInterpolation();
  void CressmanInterpolation();
  float trilinear(const float &x, const float &y,
		 const float &z, const int &param);
  void writeAsi();
  
 private:

  float latReference;
  float lonReference;

  QString outFileName;
  float* relDist;
  
  class goodRef {
   public:
    float refValue;
    float x;
    float y;
    float z;
	float rg;
	float az;
	float el;
  };
  class goodVel {
   public:
    float velValue;
    float swValue;
    float x;
    float y;
    float z;
	float rg;
	float az;
	float el;
  };

  goodRef refValues[200000];
  goodVel velValues[200000];
  long maxRefIndex;
  long maxVelIndex;
  
};
               

#endif
