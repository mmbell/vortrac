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
  zeroLat = 0;
  zeroLon = 0;

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

float* GriddedData::relEarthLocation(float *originLat, float *originLon,
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

void GriddedData::setZeroLocation(float *knownLat, float *knownLon,
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

  zeroLon = *knownLon-*relX/fac_lon;
  zeroLat = *knownLat-*relY/fac_lat;

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

void GriddedData::setCartesianReferencePoint(int ii, int jj, int kk)
{
  if((ii > iDim)||(ii < 0)||(jj > jDim)||(jj < 0)||(kk > kDim)||(kk < 0))
    Message::toScreen("GriddedData: trying to examine point outside cappi");
  refPointI = ii;
  refPointJ = jj;
  refPointK = kk;
}


void GriddedData::setAbsoluteReferencePoint(float Lat, float Lon, float Height) 
{

  // Overloaded version of setCartesianReferencePoint used when Latitude and 
  // Longitude data is known. 

  float *locations = relEarthLocation(&zeroLat, &zeroLon, &Lat, &Lon);
  // Floor is used to round to the nearest integer
  refPointI = floor(locations[0]/iGridsp +.5);
  refPointJ = floor(locations[1]/jGridsp +.5);
  refPointK = floor(Height/kGridsp +.5);
  // testing Message::toScreen("I = "+QString().setNum(refPointI)+" J = "+QString().setNum(refPointJ)+" K = "+QString().setNum(refPointK));

}


int GriddedData::getFieldIndex(QString& fieldName)
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
	  if((k*kGridsp <= (height+cylindricalHeightSpacing/2.))
	     && (k*kGridsp > (height-cylindricalHeightSpacing/2.))) {
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
	  if((k*kGridsp <= (height+cylindricalHeightSpacing/2.))
	     && (k*kGridsp > (height-cylindricalHeightSpacing/2.))) {
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
	  if((k*kGridsp <= (height+cylindricalHeightSpacing/2.))
	     && (k*kGridsp > (height-cylindricalHeightSpacing/2.))) {
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
	  if((k*kGridsp <= (height+cylindricalHeightSpacing/2))
	     && (k*kGridsp > (height-cylindricalHeightSpacing/2))) {
	    count++;
	  }
	}
      }
    }    
  }
  return count;
  
}

float* GriddedData::getCylindricalAzimuthData(QString& fieldName, 
					      float radius, float height)
{
  int numPoints = getCylindricalAzimuthLength(radius, height);
  int field = getFieldIndex(fieldName);

  float *values = new float[numPoints];

  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for(int k = 0; k < kDim; k ++) {
	float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	if((r <= (radius+cylindricalRadiusSpacing/2.)) 
	   && (r > (radius-cylindricalRadiusSpacing/2.))) {
	  if((k*kGridsp <= (height+cylindricalHeightSpacing/2))
	     && (k*kGridsp > (height-cylindricalHeightSpacing/2))) {
	    values[count] = dataGrid[field][i][j][k];
	    count++;
	  }
	}
      }    
    }
  }
  return values;
}

float* GriddedData::getCylindricalAzimuthPosition(float radius, float height) 
{
  int numPoints = getCylindricalAzimuthLength(radius, height);

  float *positions = new float[numPoints];
  
  int count = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for(int k = 0; k < kDim; k ++) {
	float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
	if((r <= (radius+cylindricalRadiusSpacing/2.)) 
	   && (r > (radius-cylindricalRadiusSpacing/2.))) {
	  if((k*kGridsp <= (height+cylindricalHeightSpacing/2))
	     && (k*kGridsp > (height-cylindricalHeightSpacing/2))) {
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
