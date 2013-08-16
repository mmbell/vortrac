/*
 *  VortexThread.h
 *  vortrac
 *
 *  Created by Michael Bell on 2/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef VORTEXTHREAD_H
#define VORTEXTHREAD_H

#include <QSize>
#include <QObject>

#include "IO/Message.h"
#include "Config/Configuration.h"
#include "DataObjects/GriddedData.h"
#include "VTD/GBVTD.h"
#include "DataObjects/Coefficient.h"
#include "DataObjects/VortexList.h"
#include "DataObjects/VortexData.h"
#include "Pressure/PressureList.h"
#include "Radar/RadarData.h"

class VortexThread : public QObject
{
  Q_OBJECT
    
 public:
  VortexThread(QObject *parent = 0);
  ~VortexThread();
  void getWinds(Configuration *wholeConfig, GriddedData *dataPtr, RadarData *radarPtr, VortexData *vortexPtr, PressureList *pressurePtr);
  void run();
    void setEnvPressure(const float& pressure) { envPressure = pressure; }
    void setOuterRadius(const float& radius) { maxObRadius = radius; }
    
 public slots:
     void catchLog(const Message& message);
   
 signals:
     void windsFound();
     void pressureFound();
     void log(const Message& message);
 
 private:
     GriddedData *gridData;
     RadarData *radarVolume;
     VortexData *vortexData;
     PressureList *pressureList;
     Configuration *configData;
     
     float* dataGaps;
     GBVTD* vtd;

     QString vortexPath;
     QString geometry;
     QString refField;
     QString velField;
     QString closure;
     float gradientHeight;
     float firstLevel;
     float lastLevel;
     float firstRing;
     float lastRing;
     float ringWidth;
     int maxWave;
     float maxObRadius;
     float maxObTimeDiff;
     float hvvpResult;
     float hvvpUncertainty;
     float envPressure;
     float outerRadius;
     int numEstimates;
     QList<PressureData> _presObs;

     float vtdStdDev;
     float convergingCenters;
     float rhoBar[16];
     void archiveWinds(float radius,int height,int maxCoeffs, Coefficient *vtdCoeffs);
     void archiveWinds(VortexData& data, float& radius,int& height,int& maxCoeffs, Coefficient *vtdCoeffs);
     // void getPressureDeficit(const float& height);
     void getPressureDeficit(VortexData* data, float* pDeficit,const float& height);
     //void calcCentralPressure();
     void calcCentralPressure(VortexData* vortex, float* pD, float height);
     void calcPressureUncertainty(float setLimit, QString nameAddition);
     void storePressureUncertaintyData(QString& fileLocation);
     void readInConfig();
     bool calcHVVP(bool printOutput);
	 void getMaxSfcWind(VortexData* data);
	 float fixAngle(float& angle);

};

#endif
