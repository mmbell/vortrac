/*
 *  RadarData.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "RadarData.h"
#include <math.h>

RadarData::RadarData(QString radarname, float lat, float lon, QString filename)
{
        Pi = 3.141592653589793238462643;
        deg2rad = Pi/180.;
        rad2deg = 180./Pi;
	radarName = radarname;
	radarLat = lat;
	radarLon = lon;
	radarFile.setFileName(filename);
}

RadarData::~RadarData()
{
  delete [] Sweeps;
  delete [] Rays;
}

bool RadarData::readVolume()
{
  
  // Virtual function
 return false;
 
}

Sweep* RadarData::getSweep(int index)
{

  return &Sweeps[index];

}

Ray* RadarData::getRay(int index)
{
  // do we want to do error checking here?
  // if(index > numRays) ErrorMessage/ return 0;
  return &Rays[index];

}

int RadarData::getNumSweeps()
{
  
  return numSweeps;

}

int RadarData::getNumRays()
{

  return numRays;

}

QString RadarData::getDateTimeString()
{

  return radarDateTime.toString(Qt::ISODate);
	
}

QDateTime RadarData::getDateTime()
{
	
	return radarDateTime;
	
}

float* RadarData::getRadarLat()
{

  return &radarLat;

}

float* RadarData::getRadarLon()
{

  return &radarLon;

}

float RadarData::radarBeamHeight(float &distance, float elevation)
{

  const float REarth = 6370.0;
  const float curve = 1.333;
  const float RE = curve * REarth;
  const float REsq = RE * RE;

  float elevRadians = elevation * acos(-1.0) / 180.0;
  float height = sqrt(distance * distance + REsq 
		      + 2.0 * distance * RE * sin(elevRadians)) - RE;
  // height+=altitude;

  return height; 
  // returns height in km
}

float RadarData::absoluteRadarBeamHeight(float &distance, float elevation)
{
  return  radarBeamHeight(distance, elevation)+altitude;
}


void RadarData::setAltitude(const float newAltitude)
{
  altitude = newAltitude;
}
