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

  // Setting default values
  sphericalRangeSpacing = 1.0;
  sphericalAzimuthSpacing = 1.0;
  sphericalElevationSpacing = 1.0;
  cylindricalRadiusSpacing = 1.0;
  cylindricalAzimuthSpacing = 1.0;
  cylindricalHeightSpacing = 1.0;

  refPointI = 0;
  refPointJ = 0;
  refPointK = 0;
  originLat = 0;
  originLon = 0;

  iGridsp = 0;
  jGridsp = 0;
  kGridsp = 0;

  //test();
}

GriddedData::~GriddedData()
{
}

void GriddedData::writeAsi()
{
}

void GriddedData::setIdim(const int& dim)
{
  iDim = dim;
}

void GriddedData::setJdim(const int& dim)
{
  jDim = dim;
}

void GriddedData::setKdim(const int& dim)
{ 
  kDim = dim;
}

void GriddedData::setIGridsp(const float& iSpacing)
{
  iGridsp = iSpacing;
}
void GriddedData::setJGridsp(const float& jSpacing)
{
  jGridsp = jSpacing;
}
void GriddedData::setKGridsp(const float& kSpacing)
{
  kGridsp = kSpacing;
}

void GriddedData::setLatLonOrigin(float *knownLat, float *knownLon,
		    float *relX, float *relY)
{
  // takes a Lat Lon point and its cooresponding grid coordinates in km
  // to set zero Lat Lon coordinates for later use.
  
  // Thanks to Peter Dodge for some code used here
  float knownLatRadians = *knownLat * acos(-1.0)/180.0;
  float fac_lat = 111.13209 - 0.56605 * cos(2.0 * knownLatRadians)
    + 0.00012 * cos(4.0 * knownLatRadians) - 0.000002 * cos(6.0 * knownLatRadians);
  float fac_lon = 111.41513 * cos(knownLatRadians)
    - 0.09455 * cos(3.0 * knownLatRadians) + 0.00012 * cos(5.0 * knownLatRadians);

  originLon = *knownLon-*relX/fac_lon;
  originLat = *knownLat-*relY/fac_lat;

  // testing Message::toScreen("Set Zero: ZeroLat = "+QString().setNum(zeroLat)+" ZeroLon = "+QString().setNum(zeroLon));
  

}

float GriddedData::fixAngle(float angle) {
  // Takes and angle in radians and puts it in the 0-2Pi range
  
  float fixangle = angle;
  if (fabs(angle) < 0.000001) { fixangle=0.; }
  if (angle > (2.* Pi)) 
    { fixangle = angle-(2.*Pi); }
  if (angle < 0.)  
    { fixangle=angle+(2.*Pi); }
  return (fixangle);

}


void GriddedData::setReferencePoint(int ii, int jj, int kk)
{
  if((ii > iDim)||(ii < 0)||(jj > jDim)||(jj < 0)||(kk > kDim)||(kk < 0))
    Message::toScreen("GriddedData: trying to examine point outside cappi");
  refPointI = ii;
  refPointJ = jj;
  refPointK = kk;
}


void GriddedData::setCartesianReferencePoint(float ii, float jj, float kk)
{
  //Message::toScreen("Setting Cartesian Reference Point");
  refPointI = int(floor(ii/iGridsp - xmin+.5));
  refPointJ = int(floor(jj/jGridsp - ymin+.5));
  refPointK = int(floor(kk/kGridsp - zmin+.5));
  //  Message::toScreen("idim = "+QString().setNum(iDim)+" jdim "+QString().setNum(jDim)+" kdim "+QString().setNum(kDim));
  //  Message::toScreen("refPointI = "+QString().setNum(refPointI)+" refPointJ = "+QString().setNum(refPointJ)+" refPointK = "+QString().setNum(refPointK));
  //  Message::toScreen("iGridsp = "+QString().setNum(iGridsp)+" jGridsp = "+QString().setNum(jGridsp)+" kGridSp = "+QString().setNum(kGridsp));
  if((refPointI > iDim)||(refPointI < 0) || 
     (refPointJ > jDim)||(refPointJ < 0) ||
     (refPointK > kDim)||(refPointK < 0))
    Message::toScreen("GriddedData: trying to examine point outside cappi");
  /*
   * Why is this giving errors here? I thought we set this up so that we 
   * attached the radar position with the radar lat lon which is typically 
   * our of cappi range?
   *
   */
}


void GriddedData::setAbsoluteReferencePoint(float Lat, float Lon, float Height) 
{
  // Overloaded version of setCartesianReferencePoint used when Latitude and 
  // Longitude data is known. 

  float *locations = getCartesianPoint(&originLat, &originLon, &Lat, &Lon);
  // Floor is used to round to the nearest integer
  refPointI = int(floor(locations[0]/iGridsp - xmin +.5));
  refPointJ = int(floor(locations[1]/jGridsp - ymin +.5));
  refPointK = int(floor(Height/kGridsp - zmin +.5));
  // testing Message::toScreen("I = "+QString().setNum(refPointI)+" J = "+QString().setNum(refPointJ)+" K = "+QString().setNum(refPointK));
  delete[] locations;
  
}

float* GriddedData::getCartesianPoint(float *Lat, float *Lon,
				float *relLat, float *relLon)
{

  // Thanks to Peter Dodge for some code used here
  float LatRadians = *Lat * acos(-1.0)/180.0;
  float fac_lat = 111.13209 - 0.56605 * cos(2.0 * LatRadians)
    + 0.00012 * cos(4.0 * LatRadians) - 0.000002 * cos(6.0 * LatRadians);
  float fac_lon = 111.41513 * cos(LatRadians)
    - 0.09455 * cos(3.0 * LatRadians) + 0.00012 * cos(5.0 * LatRadians);

  float relX = (*relLon - *Lon) * fac_lon;
  float relY = (*relLat - *Lat) * fac_lat;

  float *relArray = new float[2];
  relArray[0] = relX;
  relArray[1] = relY;
  return relArray;

  // This value is returned in KM ???? -LM

}

float GriddedData::getCartesianDistance(float *Lat, float *Lon,
										float *relLat, float *relLon)
{
	
	// Thanks to Peter Dodge for some code used here
	float LatRadians = *Lat * acos(-1.0)/180.0;
	float fac_lat = 111.13209 - 0.56605 * cos(2.0 * LatRadians)
		+ 0.00012 * cos(4.0 * LatRadians) - 0.000002 * cos(6.0 * LatRadians);
	float fac_lon = 111.41513 * cos(LatRadians)
		- 0.09455 * cos(3.0 * LatRadians) + 0.00012 * cos(5.0 * LatRadians);
	
	float relX = (*relLon - *Lon) * fac_lon;
	float relY = (*relLat - *Lat) * fac_lat;
	float dist = sqrt(relX * relX + relY * relY);
	return dist;
	
	// This value is returned in KM ???? -LM
	
}


// These functions return indices
float GriddedData::getRefPointI ()
{
	return refPointI;
}

float GriddedData::getRefPointJ ()
{
	return refPointJ;
}

float GriddedData::getRefPointK ()
{
	return refPointK;
}

// These functions return cartesian points
float GriddedData::getCartesianRefPointI ()
{
	return (refPointI + xmin)*iGridsp;
}

float GriddedData::getCartesianRefPointJ ()
{
	return (refPointJ + ymin)*jGridsp;
}

float GriddedData::getCartesianRefPointK ()
{
	return (refPointK + zmin)*kGridsp;
}

// These functions convert between indices and cartesian points
float GriddedData::getCartesianPointFromIndexI (const float& indexI)
{
	return (indexI + xmin)*iGridsp;
}

float GriddedData::getCartesianPointFromIndexJ (const float& indexJ)
{
	return (indexJ + ymin)*jGridsp;
}

float GriddedData::getCartesianPointFromIndexK (const float& indexK)
{
	return (indexK + zmin)*kGridsp;
}

// These functions convert between cartesian points and indices
float GriddedData::getIndexFromCartesianPointI (const float& cartI)
{
	return (cartI/iGridsp) - xmin;
}

float GriddedData::getIndexFromCartesianPointJ (const float& cartJ)
{
	return (cartJ/jGridsp) - ymin;
}

float GriddedData::getIndexFromCartesianPointK (const float& cartK)
{
	return (cartK/kGridsp) - zmin;
}

int GriddedData::getFieldIndex(const QString& fieldName) const
{
  int field;
  if((fieldName == "dz")||(fieldName == "DZ"))
    field = 0;
  if((fieldName == "ve")||(fieldName == "VE"))
    field = 1;
  if((fieldName == "sw")||(fieldName == "SW"))
    field = 2;
  
  return field;
}

float GriddedData::getIndexValue(QString& fieldName, float& ii, float& jj, float& kk) const
{
	if((ii > iDim)||(ii < 0)||(jj > jDim)||(jj < 0)||(kk > kDim)||(kk < 0))
		return -999.;
	int field = getFieldIndex(fieldName);
	return dataGrid[field][(int)ii][(int)jj][(int)kk];
	
}

float* GriddedData::getCartesianXslice(const QString& fieldName, 
				       const float& y, const float& z)
{
  /*
   * Returns a list of interpolated vales to match the fieldName,y,z values 
   * given for the entire range x values with the gridded data. The number 
   * of points returned should be equivalent to the dimension in the 
   * unspecified coordinate direction, in this case the x direction.
   *
   */
   
  int field = getFieldIndex(fieldName); 
  float* values = new float[(int)iDim];

  float yIndex = getIndexFromCartesianPointJ(y);
  float zIndex = getIndexFromCartesianPointK(z);
  int yMin = int(floor(yIndex));
  int yMax = int(floor(yIndex)+1);
  int zMin = int(floor(zIndex));
  int zMax = int(floor(zIndex)+1);
  float yMinDiff = yIndex - yMin;
  float yMaxDiff = yMax - yIndex;
  float zMinDiff = zIndex - zMin;
  float zMaxDiff = zMax - zIndex;

  for(int i = 0; i < iDim; i++) {
    float ave = 0;
    ave += (1-yMaxDiff)*(1-zMinDiff)*dataGrid[field][i][yMax][zMin];
    ave += (1-yMinDiff)*(1-zMinDiff)*dataGrid[field][i][yMin][zMin];
    ave += (1-yMaxDiff)*(1-zMaxDiff)*dataGrid[field][i][yMax][zMax];
    ave += (1-yMinDiff)*(1-zMaxDiff)*dataGrid[field][i][yMin][zMax];
    values[i] = ave;
  }
  return values;
}

float* GriddedData::getCartesianYslice(const QString& fieldName,
				       const float& x, const float& z)
{
  /*
   * Returns a list of interpolated vales to match the fieldName,x,z values 
   * given for the entire range y values with the gridded data. The number 
   * of points returned should be equivalent to the dimension in the 
   * unspecified coordinate direction, in this case the y direction.
   *
   */
   
  int field = getFieldIndex(fieldName); 
  float* values = new float[(int)jDim];

  float xIndex = getIndexFromCartesianPointI(x);
  float zIndex = getIndexFromCartesianPointK(z);
  int xMin = int(floor(xIndex));
  int xMax = int(floor(xIndex)+1);
  int zMin = int(floor(zIndex));
  int zMax = int(floor(zIndex)+1);

  float xMinDiff = xIndex - xMin;
  float xMaxDiff = xMax - xIndex;
  float zMinDiff = zIndex - zMin;
  float zMaxDiff = zMax - zIndex;

  for(int j = 0; j < jDim; j++) {
    float ave = 0;
    ave += (1-xMinDiff)*(1-zMaxDiff)*dataGrid[field][xMin][j][zMax];
    ave += (1-xMaxDiff)*(1-zMaxDiff)*dataGrid[field][xMax][j][zMax];
    ave += (1-xMinDiff)*(1-zMinDiff)*dataGrid[field][xMin][j][zMin];
    ave += (1-xMaxDiff)*(1-zMinDiff)*dataGrid[field][xMax][j][zMin];
    values[j] = ave;
  }
  return values;
}

float* GriddedData::getCartesianZslice(const QString& fieldName,
				       const float& x, const float& y)
{
  /*
   * Returns a list of interpolated vales to match the fieldName,x,y values 
   * given for the entire range z values with the gridded data. The number 
   * of points returned should be equivalent to the dimension in the 
   * unspecified coordinate direction, in this case the z direction.
   *
   */
   
  int field = getFieldIndex(fieldName); 
  float* values = new float[(int)kDim];

  float yIndex = getIndexFromCartesianPointJ(y);
  float xIndex = getIndexFromCartesianPointI(x);
  int yMin = int(floor(yIndex));
  int yMax = int(floor(yIndex)+1);
  int xMin = int(floor(xIndex));
  int xMax = int(floor(xIndex)+1);
  float yMinDiff = yIndex - yMin;
  float yMaxDiff = yMax - yIndex;
  float xMinDiff = xIndex - xMin;
  float xMaxDiff = xMax - xIndex;

  for(int k = 0; k < kDim; k++) {
    float ave = 0;
    ave += (1-yMinDiff)*(1-xMaxDiff)*dataGrid[field][xMax][yMin][k];
    ave += (1-yMaxDiff)*(1-xMaxDiff)*dataGrid[field][xMax][yMax][k];
    ave += (1-yMinDiff)*(1-xMinDiff)*dataGrid[field][xMin][yMin][k];
    ave += (1-yMaxDiff)*(1-xMinDiff)*dataGrid[field][xMin][yMax][k];
    values[k] = ave;
  }
  return values;
}

float GriddedData::getCartesianValue(const QString& fieldName, const float& x, 
				     const float& y, const float& z)
{

  /* 
   * This returns the field value associated witht the field name that 
   * matches the x & y & z coordinates most closely, interpolating between
   * data points. 
   *
   */

  int field = getFieldIndex(fieldName); 

  float yIndex = getIndexFromCartesianPointJ(y);
  float xIndex = getIndexFromCartesianPointI(x);
  float zIndex = getIndexFromCartesianPointK(z);
  int zMin = int(floor(zIndex));
  int zMax = int(floor(zIndex)+1);
  int yMin = int(floor(yIndex));
  int yMax = int(floor(yIndex)+1);
  int xMin = int(floor(xIndex));
  int xMax = int(floor(xIndex)+1);
  float zMinDiff = zIndex - zMin;
  float zMaxDiff = zMax -zIndex;
  float yMinDiff = yIndex - yMin;
  float yMaxDiff = yMax - yIndex;
  float xMinDiff = xIndex - xMin;
  float xMaxDiff = xMax - xIndex;

  float ave = 0;
  ave += (1-yMinDiff)*(1-xMaxDiff)*(1-zMinDiff)*dataGrid[field][xMax][yMin][zMin];
  ave += (1-yMaxDiff)*(1-xMaxDiff)*(1-zMinDiff)*dataGrid[field][xMax][yMax][zMin];
  ave += (1-yMinDiff)*(1-xMinDiff)*(1-zMinDiff)*dataGrid[field][xMin][yMin][zMin];
  ave += (1-yMaxDiff)*(1-xMinDiff)*(1-zMinDiff)*dataGrid[field][xMin][yMax][zMin];
  ave += (1-yMinDiff)*(1-xMaxDiff)*(1-zMaxDiff)*dataGrid[field][xMax][yMin][zMax];
  ave += (1-yMaxDiff)*(1-xMaxDiff)*(1-zMaxDiff)*dataGrid[field][xMax][yMax][zMax];
  ave += (1-yMinDiff)*(1-xMinDiff)*(1-zMaxDiff)*dataGrid[field][xMin][yMin][zMax];
  ave += (1-yMaxDiff)*(1-xMinDiff)*(1-zMaxDiff)*dataGrid[field][xMin][yMax][zMax];
  return ave;

}

int GriddedData::getSphericalRangeLength(float azimuth, float elevation)
{
  int count = 0;
 
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for (int k = 0; k < kDim; k++) {
	float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
	if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
	  
	  // QString test("pElevation " +QString().setNum( pElevation )+ " elevation " +QString().setNum( elevation )+ "sphericalElevationSpacing " +QString().setNum( sphericalElevationSpacing )+ "\n");
	  //Message::toScreen(test);
	  if((pElevation <=(elevation+sphericalElevationSpacing/2.))
	     && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
	    count++;
	  }
	}
      }    
    }
  }
  return count;
}

float* GriddedData::getSphericalRangeData(QString& fieldName, float azimuth, 
					  float elevation)
{    
  int numPoints = getSphericalRangeLength(azimuth, elevation);
  int field = getFieldIndex(fieldName);
  float *values = new float[numPoints];

  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j++) {
      for (int k = 0; k < kDim; k++) {
	float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
	if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
	  if((pElevation <=(elevation+sphericalElevationSpacing/2.)) 
	     && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
	    values[count] = dataGrid[field][i][j][k];
	    count++;
	  }
	}
      }							    
    }
  }
  return values;
}

float* GriddedData::getSphericalRangePosition(float azimuth, float elevation)
{
  int numPoints = getSphericalRangeLength(azimuth, elevation);
  
  float *positions = new float[numPoints];

  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j++) {
      for (int k = 0; k < kDim; k++) {
	float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
	if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
	  if((pElevation <=(elevation+sphericalElevationSpacing/2.)) 
	     && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
	    positions[count] = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
	    count++;
	  }
	}
      }							    
    }
  }
  return positions;
}


int GriddedData::getSphericalAzimuthLength(float range, float elevation)
{
  int count = 0;
  
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for (int k = 0; k < kDim; k++) {
	float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
	float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
	if((r <= (range+sphericalRangeSpacing/2.)) 
	   && (r > (range-sphericalRangeSpacing/2.))) {
	  if((pElevation <=(elevation+sphericalElevationSpacing/2.))
	     && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
	    count++;
	  }
	}
      }    
    }
  }
  return count;
}

float* GriddedData::getSphericalAzimuthData(QString& fieldName, 
					    float range, float elevation)
{
  int numPoints = getSphericalAzimuthLength(range, elevation);
  int field = getFieldIndex(fieldName);
  float *values = new float[numPoints];
  
  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for (int k = 0; k < kDim; k++) {
	float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
	float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
	if((r <= (range+sphericalRangeSpacing/2.)) 
	   && (r > (range-sphericalRangeSpacing/2.))) {
	  if((pElevation <=(elevation+sphericalElevationSpacing/2.))
	     && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
	    values[count] = dataGrid[field][i][j][k];
	    count++;
	  }
	}
      }    
    }
  }
  return values;
}

float* GriddedData::getSphericalAzimuthPosition(float range, float elevation)
{
  int numPoints = getSphericalAzimuthLength(range, elevation);
  float *positions = new float[numPoints];
  
  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for (int k = 0; k < kDim; k++) {
	float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
	float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
	if((r <= (range+sphericalRangeSpacing/2.)) 
	   && (r > (range-sphericalRangeSpacing/2.))) {
	  if((pElevation <=(elevation+sphericalElevationSpacing/2.))
	     && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
	    float azimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	    positions[count] = azimuth;
	    count++;
	  }
	}
      }    
    }
  }
  return positions;
}

int GriddedData::getSphericalElevationLength(float range, float azimuth)
{
  int count = 0;
  
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for (int k = 0; k < kDim; k++) {
	float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
	float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
	  if((r <= (range+sphericalRangeSpacing/2.)) 
	     && (r > (range-sphericalRangeSpacing/2.))) {
	    count++;
	  }
	}
      }    
    }
  }
  return count;
  
}

float* GriddedData::getSphericalElevationData(QString& fieldName, float range, 
					      float azimuth)
{
  int numPoints = getSphericalElevationLength(range, azimuth);
  int field = getFieldIndex(fieldName);
  float *values = new float[numPoints];

  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for (int k = 0; k < kDim; k++) {
	float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
	float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
	  if((r <= (range+sphericalRangeSpacing/2.)) 
	     && (r > (range-sphericalRangeSpacing/2.))) {
	    values[count] = dataGrid[field][i][j][k];
	    count++;
	  }
	}
      }    
    }
  }
  return values;
}

float* GriddedData::getSphericalElevationPosition(float range, float azimuth)
{
  int numPoints = getSphericalElevationLength(range, azimuth);
  float *positions = new float[numPoints];
  
  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for (int k = 0; k < kDim; k++) {
	float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
	float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
	  if((r <= (range+sphericalRangeSpacing/2.)) 
	     && (r > (range-sphericalRangeSpacing/2.))) {
	    positions[count] = fixAngle(atan2((k-refPointK),rp))*rad2deg;
	    count++;
	  }
	}
      }    
    }
  }
  return positions;
}




int GriddedData::getCylindricalRadiusLength(float azimuth, float height)
{
  int count = 0;
 
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for(int k = 0; k < kDim; k ++) {
	float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	if((pAzimuth <= (azimuth+cylindricalAzimuthSpacing/2.)) 
	   && (pAzimuth > (azimuth-cylindricalAzimuthSpacing/2.))) {
	  if((k*kGridsp <= ((height/kGridsp)-zmin+cylindricalHeightSpacing/2.))
	     && (k*kGridsp > ((height/kGridsp)-zmin-cylindricalHeightSpacing/2.))) {
	    count++;
	  }
	}    
      }
    }
  }
  return count;
}

float* GriddedData::getCylindricalRadiusData(QString& fieldName, float azimuth, 
			       float height)
{
  int numPoints = getCylindricalRadiusLength(azimuth, height);
  int field = getFieldIndex(fieldName);
  float *values = new float[numPoints];
  
  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j++) {
      for(int k = 0; k < kDim; k++) {
	float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	if((pAzimuth <= (azimuth+cylindricalAzimuthSpacing/2.)) 
	 && (pAzimuth > (azimuth-cylindricalAzimuthSpacing/2.))) {
	  if((k*kGridsp <= ((height/kGridsp)-zmin+cylindricalHeightSpacing/2.))
	     && (k*kGridsp > ((height/kGridsp)-zmin-cylindricalHeightSpacing/2.))) {
	    values[count] = dataGrid[field][i][j][k];
	    count++;
	  }
	}
      }     
    }
  }
  return values;
}

float* GriddedData::getCylindricalRadiusPosition(float azimuth, float height)
{
 int numPoints = getCylindricalRadiusLength(azimuth, height);
  
  float *positions = new float[numPoints];

  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j++) {
      for(int k = 0; k < kDim; k++) {
	float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
	if((pAzimuth <= (azimuth+cylindricalAzimuthSpacing/2.)) 
	   && (pAzimuth > (azimuth-cylindricalAzimuthSpacing/2.))) {
	  if((k*kGridsp <= ((height/kGridsp)-zmin+cylindricalHeightSpacing/2.))
	     && (k*kGridsp > ((height/kGridsp)-zmin-cylindricalHeightSpacing/2.))) {
	    positions[count] = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	    count++;
	  }
	}
      }						    
    }
  }
  return positions;
}

int GriddedData::getCylindricalAzimuthLength(float radius, float height)
{
  int count = 0;
  
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {  
      for(int k = 0; k < kDim; k ++) {
	float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	if((r <= (radius+cylindricalRadiusSpacing/2.)) 
	   && (r > (radius-cylindricalRadiusSpacing/2.))) {
	  if((k*kGridsp <= ((height/kGridsp)-zmin+cylindricalHeightSpacing/2))
	     && (k*kGridsp > ((height/kGridsp)-zmin-cylindricalHeightSpacing/2))) {
	    count++;
	  }
	}
      }
    }    
  }
  return count;
  
}

void GriddedData::getCylindricalAzimuthData(QString& fieldName, int numPoints,
				     float radius, float height, float* values)
{
  //  int numPoints = getCylindricalAzimuthLength(radius, height);
  int field = getFieldIndex(fieldName);

  //  float *values = new float[numPoints];

  int count = 0;
  float r = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for(int k = 0; k < kDim; k ++) {
	r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	if((r <= (radius+cylindricalRadiusSpacing/2.)) 
	   && (r > (radius-cylindricalRadiusSpacing/2.))) {
	  if((k*kGridsp <= ((height/kGridsp)-zmin+cylindricalHeightSpacing/2))
	     && (k*kGridsp > ((height/kGridsp)-zmin-cylindricalHeightSpacing/2))) {
	    values[count] = dataGrid[field][i][j][k];
	    count++;
	  }
	}
      }    
    }
  }
  //return values;
}

void GriddedData::getCylindricalAzimuthPosition(int numPoints, float radius, float height, float* positions) 
{
//  int numPoints = getCylindricalAzimuthLength(radius, height);

//  float *positions = new float[numPoints];
  
  int count = 0;
  float r = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for(int k = 0; k < kDim; k ++) {
		  r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI) 
				   + jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
		  if((r <= (radius+cylindricalRadiusSpacing/2.)) 
			 && (r > (radius-cylindricalRadiusSpacing/2.))) {
			  if((k*kGridsp <= ((height/kGridsp)-zmin+cylindricalHeightSpacing/2))
				 && (k*kGridsp > ((height/kGridsp)-zmin-cylindricalHeightSpacing/2))) {
				  float azimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
				  if (count > numPoints) {
					  // Memory overflow, bail out
					  return;
				  } else {					
					  positions[count] = azimuth;
					  count++;
				  }
			  }	
		  }
	  }    
	}
  }
}

int GriddedData::getCylindricalHeightLength(float radius, float azimuth)
{
  int count = 0;
  for(int i = 0; i < iDim; i++){
    for(int j = 0; j < jDim; j++){
      float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
      float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
      if((r <= (radius+cylindricalRadiusSpacing/2.)) 
	 && (r > (radius-cylindricalRadiusSpacing/2.))) {
	if((pAzimuth <= azimuth+cylindricalAzimuthSpacing/2.)
	   && (pAzimuth > azimuth-cylindricalAzimuthSpacing/2.)) {
	  for(int k = 0; k < kDim; k++){
	    count++;
	  }
	}
      }
    }
  }
  return count;
}

float* GriddedData::getCylindricalHeightData(QString& fieldName, float radius, 
					     float azimuth)
{
  int numPoints = getCylindricalHeightLength(radius, azimuth);
  
  int field = getFieldIndex(fieldName); 
  
  float *data = new float[numPoints];
  
  int count = 0;
  for(int i = 0; i < iDim; i++){
    for(int j = 0; j < jDim; j++){
      float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
      float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
      if((r <= (radius+cylindricalRadiusSpacing/2.)) 
	 && (r > (radius-cylindricalRadiusSpacing/2.))) {
	if((pAzimuth <= azimuth+cylindricalAzimuthSpacing/2.)
	   && (pAzimuth > azimuth-cylindricalAzimuthSpacing/2.)) {
	  for(int k = 0; k < kDim; k++){
	    data[count] = dataGrid[field][i][j][k];
	    count++;
	  }
	}
      }
    }
  }
  return data;
}

float* GriddedData::getCylindricalHeightPosition(float radius, float azimuth)
{
  int numPoints = getCylindricalHeightLength(radius, azimuth);
  float *positions = new float[numPoints];
  
  int count = 0;
  for(int i = 0; i < iDim; i++){
    for(int j = 0; j < jDim; j++){
      float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
      float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
      if((r <= (radius+cylindricalRadiusSpacing/2.)) 
	 && (r > (radius-cylindricalRadiusSpacing/2.))) {
	if((pAzimuth <= azimuth+cylindricalAzimuthSpacing/2.)
	   && (pAzimuth > azimuth-cylindricalAzimuthSpacing/2.)) {
	  for(int k = 0; k < kDim; k++){
	    positions[count] = kGridsp*k;
	    count++;
	  }
	}
      }
    }
  }
  return positions;
}

bool GriddedData::test()
{
  iDim = 10;
  jDim = 10;
  kDim = 10;
  ymin = -4;
  ymax = 5;
  xmin = -4;
  xmax = 5;
  zmin = 1;
  zmax = 10;
  iGridsp = 2;
  jGridsp = 2;
  kGridsp = 1;
  for(int i = 0; i < iDim; i++) {
    for(int j = 0; j < jDim; j++) {
      for(int k = 0; k < kDim; k++) {
	for(int dataField = 0; dataField < 3; dataField++) {
	  dataGrid[dataField][i][j][k] = dataField*j;
	}
      }
    }
  }
  for(int j = 0; j < jDim; j++) {
    for(int k = 0; k < kDim; k++) {
      QString fieldName("dz");
      Message::toScreen("field Name "+fieldName+" "+QString().setNum(getFieldIndex(fieldName)));
      float *xValues = new float[int(floor(iDim))];
	xValues = getCartesianXslice(fieldName,(j+ymin)*jGridsp+.5,(k+zmin)*kGridsp);
      for(int i = 0; i < iDim; i++) {
	
	if(getCartesianValue(fieldName,(i+xmin)*iGridsp,(j+ymin)*jGridsp, (k+zmin)*kGridsp)!=0) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(xValues[i])+" from getCartesianValue");
	  Message::toScreen(message);
	}
	if(xValues[i]!=(dataGrid[0][i][j][k]+dataGrid[0][i][j+1][k])) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(xValues[i])+" actual: "+QString().setNum(dataGrid[0][i][j][k]));
	  Message::toScreen(message);
	}
      }
      fieldName = QString("ve");
      Message::toScreen("field Name "+fieldName+" "+QString().setNum(getFieldIndex(fieldName)));
      xValues = getCartesianXslice(fieldName,(j+ymin)*jGridsp, 
	 			   (k+zmin)*kGridsp);
      for(int i = 0; i < iDim; i++) {
	if(xValues[i]!=dataGrid[1][i][j][k]) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(j)+" value:"+QString().setNum(xValues[i])+" actual: "+QString().setNum(dataGrid[1][i][j][k]));
	  Message::toScreen(message);
	}
      }
      fieldName = QString("sw");
      Message::toScreen("field Name "+fieldName+" "+QString().setNum(getFieldIndex(fieldName)));
      xValues = getCartesianXslice(fieldName,(j+ymin)*jGridsp, 
					       (k+zmin)*kGridsp);
      for(int i = 0; i < iDim; i++) {
	if(xValues[i]!=dataGrid[2][i][j][k]) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(xValues[i])+" actual: "+QString().setNum(dataGrid[2][i][j][k]));
	  Message::toScreen(message);
	}
      }
      Message::toScreen("------------------------------------------------");
    }
  }
  
  for(int i = 0; i < iDim; i++) {
    for(int k = 0; k < kDim; k++) {
      QString fieldName("dz");
      float *yValues = new float[int(floor(jDim))];
      yValues = getCartesianYslice(fieldName,(i+xmin)*iGridsp, 
				   (k+zmin)*kGridsp);
      for(int j = 0; j < jDim; j++) {
	if(yValues[j]!=dataGrid[0][i][j][k]) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(yValues[j])+" actual: "+QString().setNum(dataGrid[0][i][j][k]));
	  Message::toScreen(message);
	}
      }
      fieldName = QString("ve");
      yValues = getCartesianYslice(fieldName,(i+xmin)*iGridsp, 
				   (k+zmin)*kGridsp);
      for(int j = 0; j < jDim; j++) {
	if(yValues[j]!=dataGrid[1][i][j][k]) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(yValues[j])+" actual "+QString().setNum(dataGrid[1][i][j][k]));
	  Message::toScreen(message);
	}
      }
      fieldName = QString("sw");
      yValues = getCartesianYslice(fieldName,(i+xmin)*iGridsp, 
					       (k+zmin)*kGridsp);
      for(int j = 0; j < jDim; j++) {
	if(yValues[j]!=dataGrid[2][i][j][k]) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(yValues[j])+" actual "+QString().setNum(dataGrid[2][i][j][k]));
	  Message::toScreen(message);
	}
      }
      Message::toScreen("********************************************");
    }
  }
  
  for(int i = 0; i < iDim; i++) {
    for(int j = 0; j < jDim; j++) {
      QString fieldName("dz");
      float *zValues = new float[int(floor(kDim))];
      zValues= getCartesianZslice(fieldName,(i+xmin)*iGridsp,(j+ymin)*jGridsp);
      for(int k = 0; k < kDim; k++) {
	if(zValues[k]!=dataGrid[0][i][j][k]) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(zValues[k]));
	  Message::toScreen(message);
	}
      }
      fieldName = QString("ve");
      zValues = getCartesianZslice(fieldName,(i+xmin)*iGridsp, 
				   (j+ymin)*jGridsp);
      for(int k = 0; k < kDim; k++) {
	if(zValues[k]!=dataGrid[1][i][j][k]) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(zValues[k]));
	  Message::toScreen(message);
	}
      }
      fieldName = QString("sw");
      zValues = getCartesianZslice(fieldName,(i+xmin)*iGridsp,
					       (j+ymin)*jGridsp);
      for(int k = 0; k < kDim; k++) {
	if(zValues[k]!=dataGrid[2][i][j][k]) {
	  QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(zValues[k]));
	  Message::toScreen(message);
	}
      }
    }
  } 

  Message::toScreen("Finished Checking"); 
  return true;
}
