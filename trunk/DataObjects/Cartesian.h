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
#include "Radar/RadarData.h"
#include "DataObjects/GriddedData.h"

class CartesianData : public GriddedData
{
 
 public:
  CartesianData();
  ~CartesianData();
  void gridData(RadarData *radarData, QDomElement cappiConfig,
		float *vortexLat, float *vortexLon);
  void BarnesInterpolation(RadarData *radarData);
  float bilinear(const float &x, const float &y,
		 const float &z, const int &param);

 private:
  float xmin, xmax;
  float ymin, ymax;
  float zmin, zmax;
  float cartGrid[3][256][256][20];
  char* outfile;
  float* relDist;

};
               

#endif
