/*
 * GriddedData.cpp
 * VORTRAC
 *
 * Created by Michael Bell on 07/11/05
 * Copyright 2005 University Corporation for Atmospheric Research.
 * All rights reserved.
 * 
 */


// Generic gridded data class
// To be subclassed with Cartesian, Earth, Radar,and Cylindrical
// Coordinate systems

#include "GriddedData.h"
#include <math.h>

GriddedData::GriddedData()
{

  Pi = 3.141592653589793238462643;
  deg2rad = Pi/180.;
  rad2deg = 180./Pi;

}

GriddedData::~GriddedData()
{
}

void GriddedData::gridData(RadarData *radarData, QDomElement cappiConfig,
			   float *vortexLat, float *vortexLon)
{
}

float* GriddedData::relLocation(float *originLat, float *originLon,
				float *relLat, float *relLon)
{

  // Thanks to Peter Dodge for some code used here
  float originLatRadians = *originLat * acos(-1.0)/180.0;
  float fac_lat = 111.13209 - 0.56605 * cos(2.0 * originLatRadians)
    + 0.00012 * cos(4.0 * originLatRadians) - 0.000002 * cos(6.0 * originLatRadians);
  float fac_lon = 111.41513 * cos(originLatRadians)
    - 0.09455 * cos(3.0 * originLatRadians) + 0.00012 * cos(5.0 * originLatRadians);

  float relX = (*relLon - *originLon) * fac_lon;
  float relY = (*relLat - *originLat) * fac_lat;

  float *relArray = new float[2];
  relArray[0] = relX;
  relArray[1] = relY;
  return relArray;

}

float GriddedData::fixAngle(const float &angle) {

  float fixangle;
  if (fabs(angle) < 0.000001) { fixangle=0.; }
  if (angle > 2.* Pi) { fixangle=angle-2.*Pi; }
  if (angle < 0.)  { fixangle=angle+2.*Pi; }
  return (fixangle);

}
