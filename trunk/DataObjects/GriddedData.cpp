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
  sphericalRangeSpacing = 5;
  sphericalAzimuthSpacing = .5;
  sphericalElevationSpacing = .5;
  poiX = 0;
  poiY = 0;
  poiZ = 0;

}

GriddedData::~GriddedData()
{
}

void GriddedData::writeAsi()
{
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

float GriddedData::fixAngle(const float &angle) {
  
  float fixangle;
  if (fabs(angle) < 0.000001) { fixangle=0.; }
  if (angle > 2.* Pi) { fixangle=angle-2.*Pi; }
  if (angle < 0.)  { fixangle=angle+2.*Pi; }
  return (fixangle);

}

void GriddedData::setPointOfInterest(int ii, int jj, int kk)
{
  if((ii > xDim)||(ii < 0)||(jj > yDim)||(jj < 0)||(kk > zDim)||(kk < 0))
    Message::toScreen("GriddedData: trying to examine point outside cappi");
  poiX = ii;
  poiY = jj;
  poiZ = kk;
}


int GriddedData::getFieldIndex(QString& fieldName)
{
  int field;
  if((fieldName == "rf")||(fieldName == "RF"))
    field = 1;
  if((fieldName == "ve")||(fieldName == "VE"))
    field = 2;
  if((fieldName == "sw")||(fieldName == "SW"))
    field = 3;
  
  return field;
}

int GriddedData::getSphericalRangeLength(float azimuth, float elevation)
{
  int count = 0;
 
  for(int i = 0; i < xDim; i ++) {
    for(int j = 0; j < yDim; j ++) {
      for (int k = 0; k < zDim; k++) {
	float rp = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY));
	float pAzimuth = atan2((j-poiY),(i-poiX))*rad2deg;
	float pElevation = atan2((k-poiZ),rp)*rad2deg;
	if((pAzimuth < (azimuth+sphericalAzimuthSpacing/2)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2))) {
	  if((pElevation <(elevation+sphericalElevationSpacing/2))
	     && (pElevation > (elevation-sphericalElevationSpacing/2))) {
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
  for(int i = 0; i < xDim; i ++) {
    for(int j = 0; j < yDim; j++) {
      for (int k = 0; k < zDim; k++) {
	float rp = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY));
	float pAzimuth = atan2((j-poiY),(i-poiX))*rad2deg;
	float pElevation = atan2((k-poiZ),rp)*rad2deg;
	if((pAzimuth < (azimuth+sphericalAzimuthSpacing/2)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2))) {
	  if((pElevation <(elevation+sphericalElevationSpacing/2)) 
	     && (pElevation > (elevation-sphericalElevationSpacing/2))) {
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
  for(int i = 0; i < xDim; i ++) {
    for(int j = 0; j < yDim; j++) {
      for (int k = 0; k < zDim; k++) {
	float rp = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY));
	float pAzimuth = atan2((j-poiY),(i-poiX))*rad2deg;
	float pElevation = atan2((k-poiZ),rp)*rad2deg;
	if((pAzimuth < (azimuth+sphericalAzimuthSpacing/2)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2))) {
	  if((pElevation <(elevation+sphericalElevationSpacing/2)) 
	     && (pElevation > (elevation-sphericalElevationSpacing/2))) {
	    positions[count] = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY)+(k-poiZ)*(k-poiZ));
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
  
  for(int i = 0; i < xDim; i ++) {
    for(int j = 0; j < yDim; j ++) {
      for (int k = 0; k < zDim; k++) {
	float rp = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY));
	float r = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY)+(k-poiZ)*(k-poiZ));
	float pElevation = atan2((k-poiZ),rp)*rad2deg;
	if((r < (range+sphericalRangeSpacing/2)) 
	   && (r > (range-sphericalRangeSpacing/2))) {
	  if((pElevation <(elevation+sphericalElevationSpacing/2))
	     && (pElevation > (elevation-sphericalElevationSpacing/2))) {
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
  int count = getSphericalAzimuthLength(range, elevation);
  int field = getFieldIndex(fieldName);

  float *values = new float[count];
  
  for(int i = 0; i < xDim; i ++) {
    for(int j = 0; j < yDim; j ++) {
      for (int k = 0; k < zDim; k++) {
	float rp = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY));
	float r = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY)+(k-poiZ)*(k-poiZ));
	float pElevation = atan2((k-poiZ),rp)*rad2deg;
	if((r < (range+sphericalRangeSpacing/2)) 
	   && (r > (range-sphericalRangeSpacing/2))) {
	  if((pElevation <(elevation+sphericalElevationSpacing/2))
	     && (pElevation > (elevation-sphericalElevationSpacing/2))) {
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
  int count = getSphericalAzimuthLength(range, elevation);

  float *positions = new float[count];
  
  for(int i = 0; i < xDim; i ++) {
    for(int j = 0; j < yDim; j ++) {
      for (int k = 0; k < zDim; k++) {
	float rp = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY));
	float r = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY)+(k-poiZ)*(k-poiZ));
	float pElevation = atan2((k-poiZ),rp)*rad2deg;
	if((r < (range+sphericalRangeSpacing/2)) 
	   && (r > (range-sphericalRangeSpacing/2))) {
	  if((pElevation <(elevation+sphericalElevationSpacing/2))
	     && (pElevation > (elevation-sphericalElevationSpacing/2))) {
	    float azimuth = atan2((j-poiY),(i-poiX))*rad2deg;
	    if (azimuth < 0)
	      azimuth+=360;
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
  
  for(int i = 0; i < xDim; i ++) {
    for(int j = 0; j < yDim; j ++) {
      for (int k = 0; k < zDim; k++) {
	float r = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY)+(k-poiZ)*(k-poiZ));
	float pAzimuth = atan2((j-poiY),(i-poiX))*rad2deg;
	if((pAzimuth < (azimuth+sphericalAzimuthSpacing/2)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2))) {
	  if((r < (range+sphericalRangeSpacing/2)) 
	     && (r > (range-sphericalRangeSpacing/2))) {
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
  int count = getSphericalElevationLength(range, azimuth);
  int field = getFieldIndex(fieldName);
  
  float *values = new float[count];
  
  for(int i = 0; i < xDim; i ++) {
    for(int j = 0; j < yDim; j ++) {
      for (int k = 0; k < zDim; k++) {
	float r = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY)+(k-poiZ)*(k-poiZ));
	float pAzimuth = atan2((j-poiY),(i-poiX))*rad2deg;
	if((pAzimuth < (azimuth+sphericalAzimuthSpacing/2)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2))) {
	  if((r < (range+sphericalRangeSpacing/2)) 
	     && (r > (range-sphericalRangeSpacing/2))) {
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
  int count = getSphericalElevationLength(range, azimuth);
  
  float *positions = new float[count];
  
  for(int i = 0; i < xDim; i ++) {
    for(int j = 0; j < yDim; j ++) {
      for (int k = 0; k < zDim; k++) {
	float rp = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY));
	float r = sqrt((i-poiX)*(i-poiX)+(j-poiY)*(j-poiY)+(k-poiZ)*(k-poiZ));
	float pAzimuth = atan2((j-poiY),(i-poiX))*rad2deg;
	if((pAzimuth < (azimuth+sphericalAzimuthSpacing/2)) 
	   && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2))) {
	  if((r < (range+sphericalRangeSpacing/2)) 
	     && (r > (range-sphericalRangeSpacing/2))) {
	    positions[count] = atan2((k-poiZ),rp)*rad2deg;
	    count++;
	  }
	}
      }    
    }
  }
  return positions;
}
