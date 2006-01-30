/*
 *  Cartesian.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/29/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef CARTESIAN_H
#define CARTESIAN_H

#include <QDomElement>
#include <QFile>
#include "Radar/RadarData.h"
#include "DataObjects/GriddedData.h"

class CartesianData : public GriddedData
{
 
 public:
  CartesianData();
  ~CartesianData();
  void gridData(RadarData *radarData, QDomElement cappiConfig,
		float *vortexLat, float *vortexLon);
  void BarnesInterpolation();
  void CressmanInterpolation();
  float bilinear(const float &x, const float &y,
		 const float &z, const int &param);
  void writeAsi();
  
 private:
  float xmin, xmax;
  float ymin, ymax;
  float zmin, zmax;
  float cartGrid[3][256][256][20];
  float latReference, lonReference;
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

  goodRef refValues[20000];
  goodVel velValues[60000];
  int maxRefIndex;
  int maxVelIndex;
  
};
               

#endif
