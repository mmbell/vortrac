/*
 *  RadarData.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef RADARDATA_H
#define RADARDATA_H

#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDomElement>
#include "Sweep.h"
#include "Ray.h"

class RadarData
{

 public:

  RadarData(QString radarname, float lat, float lon, QString filename);
  virtual ~RadarData();
  virtual bool readVolume() = 0;
  Sweep* getSweep(int index);
  Ray* getRay(int index);
  int getNumRays();
  int getNumSweeps();
  bool isDealiased() { return dealiased; }
  void isDealiased(bool flag) { dealiased = flag; }
  QString getDateTimeString();
  float* getRadarLat();
  float* getRadarLon();
  float radarBeamHeight(float &distance, float elevation);


 protected:
  QString radarName;
  float radarLat;
  float radarLon;
  QDateTime radarDateTime;
  QFile radarFile;
  Sweep* Sweeps;
  Ray* Rays;
  int numSweeps;
  int numRays;

 private:
  bool dealiased;
  
};


#endif

