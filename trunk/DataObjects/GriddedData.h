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
#include "IO/Message.h"
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
  // Return a 1D array of values
  float* getNativeData();
  float* getCartesianData();
  float* getCylindricalData();
  float* getSphericalData();
  
  // Return a 1D array of locations
  float* getNativePositions();
  float* getCartesianPositions();
  float* getCylindricalPositions();
  float* getSphericalPositions();

  /* these are all done in Math Coordinates, should we changes the names,
     so the sound less like meteorological coords?  -LM */
  int getFieldIndex(QString& fieldName);

  void setPointOfInterest(int ii, int jj, int kk); 

  /* Needed a reference point before we could redo coordinate systems. -LM */

  int getSphericalRangeLength(float azimuth, float elevation);
  float* getSphericalRangeData(QString& fieldName, float azimuth, 
			       float elevation);
  float* getSphericalRangePosition(float azimuth, float elevations);
  int getSphericalAzimuthLength(float range,float elevation);
  float* getSphericalAzimuthData(QString& fieldName, float range, 
				 float elevation);
  float* getSphericalAzimuthPosition(float range, float elevation);
  int getSphericalElevationLength(float range, float elevation);
  float* getSphericalElevationData(QString& fieldName, float range, 
				   float azimuth);
  float* getSphericalElevationPosition(float range,float azimuth);


  /* All of these functions go through all points in the grid to check for
     points within the requested range. Somewhat inefficient. -LM
  */
  
  
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
  float dataGrid[3][256][256][20];
  //dataGrid[0] = reflectivity
  //dataGrid[1] = doppler velocity magnitude
  //dataGrid[2] = spectral width

  float sphericalRangeSpacing;
  float sphericalAzimuthSpacing;
  float sphericalElevationSpacing;

  float poiX;
  float poiY;
  float poiZ;
  
  /* I don't think we still need these enumeration values!?!*/

  enum coordSystems {
    cartesian,
    cylindrical,
    spherical
  };
  coordSystems coordSystem;

};

#endif
