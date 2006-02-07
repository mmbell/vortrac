/*
 *  GriddedData.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef GRIDDEDDATA_H
#define GRIDDEDDATA_H

#include "Radar/RadarData.h"
#include <QDomElement>
#include <QStringList>

class GriddedData
{

 public:
  GriddedData();
  virtual ~GriddedData();

  virtual void writeAsi() = 0;
  
  float getXdim() { return xDim; }
  float getYdim() { return yDim; }
  float getZdim() { return zDim; }
  void setXdim(const int& dim);
  void setYdim(const int& dim);
  void setZdim(const int& dim);
  int getCoordSystem() { return coordSystem; }
  float fixAngle(const float &angle);
  float* relEarthLocation(float *originLat, float *originLon,
		     float *relLat, float* relLon);
  float* absLocation(float *originLat, float *originLon,
		     float *relX, float *relY);
  // Return a 3D array of values
  float* getNativeValues();
  float* getCartesianValues();
  float* getCylindricalValues();
  float* getSphericalValues();
  
  // Return a 3D array of locations
  float* getNativeLocations();
  float* getCartesianLocations();
  float* getCylindricalLocations();
  float* getSphericalLocations();


 protected:
  float xDim;
  float yDim;
  float zDim;
  float xGridsp;
  float yGridsp;
  float zGridsp;
  float Pi;
  float deg2rad;
  float rad2deg;
  float numFields;
  QStringList fieldNames;
  
  enum coordSystems {
    cartesian,
    cylindrical,
    spherical
  };
  coordSystems coordSystem;

};

#endif
