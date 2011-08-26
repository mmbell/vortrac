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

  virtual void writeAsi(); // = 0;
  virtual bool writeAsi(const QString& fileName); // = 0;
  void setExit(volatile bool *exit);
  bool returnExitNow() const { return *exitNow; }
  
  float getIdim() const { return iDim; }
  float getJdim() const { return jDim; }
  float getKdim() const { return kDim; }
  //void setIdim(const int& dim);
  //void setJdim(const int& dim);
  //void setKdim(const int& dim);
  float getIGridsp() const { return iGridsp; }
  float getJGridsp() const { return jGridsp; }
  float getKGridsp() const { return kGridsp; }
  //void setIGridsp(const float& iSpacing);
  //void setJGridsp(const float& jSpacing);
  //void setKGridsp(const float& kSpacing);
  float fixAngle(float angle);
  void setLatLonOrigin(float *knownLat, float *knownLon, float *relX, 
		       float *relY);
  void setReferencePoint(int ii, int jj, int kk);
  void setCartesianReferencePoint(float ii, float jj, float kk); 
  void setAbsoluteReferencePoint(float Lat, float Lon, float Height);
  static float* getCartesianPoint(float *Lat, float *Lon,
				  float *relLat, float* relLon);
  static float getCartesianDistance(float *Lat, float *Lon,
				    float *relLat, float* relLon);
  static float* getAdjustedLatLon(const float& startLat, const float& startLon,
				  const float& changeInX, 
				  const float& changeInY);
  float getRefPointI();
  float getRefPointJ();
  float getRefPointK();
  
  float getCartesianRefPointI();
  float getCartesianRefPointJ();
  float getCartesianRefPointK();
  
  float getCartesianPointFromIndexI (const float& indexI);
  float getCartesianPointFromIndexJ (const float& indexJ);
  float getCartesianPointFromIndexK (const float& indexK);
  
  int getIndexFromCartesianPointI (const float& cartI);
  int getIndexFromCartesianPointJ (const float& cartJ);
  int getIndexFromCartesianPointK (const float& cartK);
  
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
  float* getSphericalRangeData(QString& fieldName, float azimuth, 
			       float elevation, int numPoints);

  float* getSphericalRangePosition(float azimuth, float elevation);
  float* getSphericalRangePosition(float azimuth, float elevation, int numPts);

  int getSphericalAzimuthLength(float range,float elevation);
  float* getSphericalAzimuthData(QString& fieldName, float range, 
				 float elevation);
  float* getSphericalAzimuthPosition(float range, float elevation);
  int getSphericalElevationLength(float range, float elevation);
  float* getSphericalElevationData(QString& fieldName, float range, 
				   float azimuth);
  float* getSphericalElevationPosition(float range,float azimuth);

   // Spherical Coordinates  !!!!Testing ONLY!!!!!

  int getSphericalRangeLengthTest(float azimuth, float elevation);
  float* getSphericalRangeDataTest(QString& fieldName, float azimuth, 
			       float elevation);
  float* getSphericalRangePositionTest(float azimuth, float elevation);

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

  // Cylindrical Coordinates !!!! Testing Only !!!!

  int getCylindricalAzimuthLengthTest2(float radius, float height);
  void getCylindricalAzimuthDataTest2(QString& fieldName,int numPoints, float radius,
				 float height, float* values);
  void getCylindricalAzimuthPositionTest2(int numPoints, float radius, float height, float* positions);

  /* All of these functions go through all points in the grid to check for
     points within the requested radius. Somewhat inefficient. -LM
  */
  
  static int getMaxFields() { return maxFields; }
  static int getMaxIDim() { return maxIDim; }
  static int getMaxJDim() { return maxJDim; }
  static int getMaxKDim() { return maxKDim; }

  float getCylindricalAzimuthSpacing() { return cylindricalAzimuthSpacing; }
  void setCylindricalAzimuthSpacing(const float& newSpacing);
  
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

  static const int maxFields = 3;
  static const int maxIDim = 256;
  static const int maxJDim = 256;
  static const int maxKDim = 20;

  float dataGrid[maxFields][maxIDim][maxJDim][maxKDim];
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

  // The mins & maxs have units of km relative to radar location
  float xmin, xmax;
  float ymin, ymax;
  // zmin & zmax have units in km relative to radar altitude
  float zmin, zmax;

  bool test();

  volatile bool *exitNow;
  
};

#endif
