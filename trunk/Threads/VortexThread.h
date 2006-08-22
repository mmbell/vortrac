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

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>

#include "IO/Message.h"
#include "Config/Configuration.h"
#include "DataObjects/GriddedData.h"
#include "VTD/GBVTD.h"
#include "DataObjects/Coefficient.h"
#include "DataObjects/VortexList.h"
#include "DataObjects/VortexData.h"
#include "ChooseCenter.h"
#include "Pressure/PressureList.h"

class VortexThread : public QThread
{
  Q_OBJECT
    
 public:
     VortexThread(QObject *parent = 0);
     ~VortexThread();
	 void getWinds(Configuration *wholeConfig, GriddedData *dataPtr, VortexData *vortexPtr, PressureList *pressurePtr);

 public slots:
     void catchLog(const Message& message);
   
 protected:
     void run();
 
 signals:
     void windsFound();
     void pressureFound();
     void log(const Message& message);
 
 private:
     QMutex mutex;
     QWaitCondition waitForData;
     bool abort;
	 GriddedData *gridData;
	 VortexData *vortexData;
	 PressureList *pressureList;
	 Configuration *configData;
	 QDomElement vtdConfig;
	 float* refLat;
	 float* refLon;
	 float* dataGaps;
	 GBVTD* vtd;
	 Coefficient* vtdCoeffs; 
	 float firstLevel;
	 float lastLevel;
	 float firstRing;
	 float lastRing;
	 float vtdStdDev;
	 float convergingCenters;
	 float rhoBar[16];
	 float centralPressure;
	 float centralPressureStdDev;
	 float* pressureDeficit;
	 void archiveWinds(float& radius,float& height,float& maxCoeffs);
	 void getPressureDeficit(const float& height);
	 void calcCentralPressure();

};

#endif
