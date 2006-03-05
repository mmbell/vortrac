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
  
  float getIdim() { return iDim; }
  float getJdim() { return jDim; }
  float getKdim() { return kDim; }
  void setIdim(const int& dim);
  void setJdim(const int& dim);
  void setKdim(const int& dim);
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

  // Spherical Coordinates

  int getSphericalRangeLength(float azimuth, float elevation);
  float* getSphericalRangeData(QString& fieldName, float azimuth, 
			       float elevation);
  float* getSphericalRangePosition(float azimuth, float elevation);
  int getSphericalAzimuthLength(float range,float elevation);
  float* getSphericalAzimuthData(QString& fieldName, float range, 
				 float elevation);
  float* getSphericalAzimuthPosition(float range, float elevation);
  int getSphericalElevationLength(float range, float elevation);
  float* getSphericalElevationData(QString& fieldName, float range, 
				   float azimuth);
  float* getSphericalElevationPosition(float range,float azimuth);

  // Cylindrical Coordinates

  int getCylindricalRangeLength(float azimuth, float height);
  float* getCylindricalRangeData(QString& fieldName, float azimuth, 
				 float height);
  float* getCylindricalRangePosition(float azimuth, float height);
  int getCylindricalAzimuthLength(float range, float height);
  float* getCylindricalAzimuthData(QString& fieldName, float range,
				   float height);
  float* getCylindricalAzimuthPosition(float range, float height);
  int getCylindricalHeightLength(float range, float height);
  float* getCylindricalHeightData(QString& fieldName, float range, 
				  float height);
  float* getCylindricalHeightPosition(float range, float height);

  /* All of these functions go through all points in the grid to check for
     points within the requested range. Somewhat inefficient. -LM
  */
  
  
 protected:
  float iDim;
  float jDim;
  float kDim;
  float iGridsp;
  float jGridsp;
  float kGridsp;
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

  float cylindricalRangeSpacing;
  float cylindricalAzimuthSpacing;
  float cylindricalHeightSpacing;

  float poiI;
  float poiJ;
  float poiK;
  
  /* I don't think we still need these enumeration values -LM */

  enum coordSystems {
    cartesian,
    cylindrical,
    spherical
  };
  coordSystems coordSystem;

};

#endif
