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
  
  float getIdim() const { return iDim; }
  float getJdim() const { return jDim; }
  float getKdim() const { return kDim; }
  void setIdim(const int& dim);
  void setJdim(const int& dim);
  void setKdim(const int& dim);
  float getIGridsp() const { return iGridsp; }
  float getJGridsp() const { return jGridsp; }
  float getKGridsp() const { return kGridsp; }
  void setIGridsp(const float& iSpacing);
  void setJGridsp(const float& jSpacing);
  void setKGridsp(const float& kSpacing);
  int getCoordSystem() { return coordSystem; }
  float fixAngle(float angle);
  void setLatLonOrigin(float *knownLat, float *knownLon, float *relX, 
		       float *relY);
  void setReferencePoint(int ii, int jj, int kk);
  void setCartesianReferencePoint(float ii, float jj, float kk); 
  void setAbsoluteReferencePoint(float Lat, float Lon, float Height);
  float* getCartesianPoint(float *Lat, float *Lon,
		     float *relLat, float* relLon);
  static float getCartesianDistance(float *Lat, float *Lon,
				    float *relLat, float* relLon);
  float getRefPointI();
  float getRefPointJ();
  float getRefPointK();
  
  float getCartesianRefPointI();
  float getCartesianRefPointJ();
  float getCartesianRefPointK();
  
  float getCartesianPointFromIndexI (const float& indexI);
  float getCartesianPointFromIndexJ (const float& indexJ);
  float getCartesianPointFromIndexK (const float& indexK);
  
  float getIndexFromCartesianPointI (const float& cartI);
  float getIndexFromCartesianPointJ (const float& cartJ);
  float getIndexFromCartesianPointK (const float& cartK);
  
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

  int getFieldIndex(const QString& fieldName) const;

  float getIndexValue(QString& fieldName, float& i, float& j, float& k) const;

  /* Needed a reference point before we could redo coordinate systems. -LM */
  // Cartesian Coordinates
  float* getCartesianXslice(const QString& fieldName, const float& y, 
			    const float& z);
  float* getCartesianYslice(const QString& fieldName, const float& x, 
			    const float& z);
  float* getCartesianZslice(const QString& fieldName, const float& x, 
			    const float& y);
  float getCartesianValue(const QString& fieldName, const float& x, 
			  const float& y, const float& z);
  
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

  int getCylindricalRadiusLength(float azimuth, float height);
  float* getCylindricalRadiusData(QString& fieldName, float azimuth, 
				 float height);
  float* getCylindricalRadiusPosition(float azimuth, float height);
  int getCylindricalAzimuthLength(float radius, float height);
  void getCylindricalAzimuthData(QString& fieldName,int numPoints, float radius,
				   float height, float* values);
  void getCylindricalAzimuthPosition(int numPoints, float radius, float height, float* positions);
  int getCylindricalHeightLength(float radius, float height);
  float* getCylindricalHeightData(QString& fieldName, float radius, 
				  float height);
  float* getCylindricalHeightPosition(float radius, float height);

  /* All of these functions go through all points in the grid to check for
     points within the requested radius. Somewhat inefficient. -LM
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

  float cylindricalRadiusSpacing;
  float cylindricalAzimuthSpacing;
  float cylindricalHeightSpacing;

  float refPointI;
  float refPointJ;
  float refPointK;

  // Latitude and Longitude Coordinates for the i = 0, j= 0, k = 0, point
  float originLat;
  float originLon;
  
  /* I don't think we still need these enumeration values -LM */

  enum coordSystems {
    cartesian,
    cylindrical,
    spherical
  };
  coordSystems coordSystem;

  float xmin, xmax;
  float ymin, ymax;
  float zmin, zmax;

  bool test();
  
};

#endif
