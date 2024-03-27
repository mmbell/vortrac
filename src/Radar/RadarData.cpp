/*
 *  RadarData.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "Radar/RadarData.h"
#include <math.h>
#include <QFile>
#include <QTextStream>
#include "IO/Message.h"

#if QT_VERSION >= 0x060000
using namespace Qt;
#endif

RadarData::RadarData(QString radarname, float lat, float lon, QString filename)
{
  Pi = 3.141592653589793238462643;
  deg2rad = Pi/180.;
  rad2deg = 180./Pi;
  radarName = radarname;
  radarLat = lat;
  radarLon = lon;
  radarFileName = filename;
  radarFile = new QFile(filename);
  if(!radarFile->exists())
    Message::toScreen("Radar File: "+filename+" does NOT exist");
  Sweeps = NULL;
  Rays = NULL;
  maxRange = 148; // default max unambiguated range. Can be overwritten in the config
  preGridded = false;
}

RadarData::~RadarData()
{
  delete radarFile;
}

bool RadarData::readVolume()
{
  
  // Virtual function
 return false;
 
}

Sweep* RadarData::getSweep(int index)
{
  if ((index >= 0) and (index < numSweeps)) {
    return &Sweeps[index];
  } else {
	return NULL;
  }
}

Ray* RadarData::getRay(int index)
{
  // do we want to do error checking here?
  // if(index > numRays) ErrorMessage/ return 0;
  if ((index >= 0) and (index < numRays)) {
    return &Rays[index];
  } else {
	return NULL;
  }
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

  const float REarth = 6371.0;
  const float RE = 4*REarth/3;
  const float REsq = RE * RE;

  float elevRadians = elevation * acos(-1.0) / 180.0;
  float sinelev = sin(elevRadians);
  //float height = sqrt(distance * distance + REsq + 2.0 * distance * RE * sin(elevRadians)) - RE;
  float top = distance*distance+2*distance*RE*sinelev;
  float bottom =  sqrt(distance*distance + REsq + 2.*distance*RE*sinelev) + RE;
  float height = top/bottom;
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


bool RadarData::writeToFile(const QString fileName)
{
  QFile* outputFile = new QFile(fileName);
  outputFile->open(QIODevice::WriteOnly);
  QTextStream out(outputFile);
  out << "vbin" << endl;
  for(int i = 0; i < numSweeps; i++) {
    out << reset << qSetRealNumberPrecision(7) << scientific << qSetFieldWidth(15) << Sweeps[i].getVel_numgates();
    out << endl;
  }
  int line = 0;
  out << "az: " << endl;
  out << numSweeps << endl;
  for(int i = 0; i < numSweeps; i++) {
    int start = Sweeps[i].getFirstRay();
    int stop = Sweeps[i].getLastRay();
    out << stop-start+1 << endl;
    for(int j = start; j <= stop; j++) {
      out << reset << qSetRealNumberPrecision(7) << scientific << qSetFieldWidth(15) << Rays[j].getAzimuth();
      line++;
      if(line == 8) {
	out << endl;
	line = 0;
      }
    }
    line = 0;
    out << endl;
  }
  out << "el: " << endl;
  out << numSweeps << endl;
  for(int i = 0; i < numSweeps; i++) {
    int start = Sweeps[i].getFirstRay();
    int stop = Sweeps[i].getLastRay();
    out << stop-start+1 << endl;
    for(int j = start; j <= stop; j++) {
      out << reset << qSetRealNumberPrecision(7) << scientific << qSetFieldWidth(15) << Rays[j].getElevation();
      line++;
      if(line==8){
	out << endl;
	line = 0;
      }
     }
    line = 0;
    out << endl;
  }
  out << "vd: " << endl;
  out << numSweeps << endl;
  for(int i = 0; i < numSweeps; i++) {
    int start = Sweeps[i].getFirstRay();
    int stop = Sweeps[i].getLastRay();
    out << stop-start+1 << endl;
    for(int j = start; j <= stop; j++) {
      float* vel_data = Rays[j].getVelData();
      out << Rays[j].getVel_numgates() << endl;
      for(int k = 0; k < Rays[j].getVel_numgates(); k++) {
	out << reset << qSetRealNumberPrecision(7) << scientific << qSetFieldWidth(15) << vel_data[k];
	line++;
	if(line==8){
	  out << endl;
	  line = 0;
	}
      }
      if(line!=0){
	line = 0;
	out <<endl;
      }
    }
    if(line!=0) {
      line = 0;
      out << endl;
    }
  }
  line = 0;
  out << endl;
  outputFile->close();
  return true;
}
 
bool RadarData::fileIsReadable()
{
  if(!radarFile->exists())
    return false;
  if(radarFile->fileName()==QString())
    return false;
  if(!radarFile->open(QIODevice::ReadOnly))
    return false;
  
  radarFile->close();
  return true;  
}
  
QString RadarData::getFileName()
{
  return radarFileName;
  //radarFile->fileName();
  /*
  if(radarFile!=NULL){
    if(radarFile->exists())
      return radarFile->fileName();
    else {
      Message::toScreen("RadarFile Does Not Exist");
      return QString();
    }
  }
  Message::toScreen("RadarFile is NULL");
  return QString();
  */
}

float RadarData::getMaxUnambig_range()
{
    if (preGridded)
      return maxRange;

    float maxRange = -999;
    for (int i = 0; i < numSweeps; ++i) {
        float range = Sweeps[i].getUnambig_range();
        if ((range > maxRange) and (Sweeps[i].getVel_numgates() > 0)) maxRange = range;
    }
    return maxRange;
}
