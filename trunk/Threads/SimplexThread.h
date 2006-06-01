/*
 *  SimplexThread.h
 *  vortrac
 *
 *  Created by Michael Bell on 2/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef SIMPLEXTHREAD_H
#define SIMPLEXTHREAD_H

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>
#include <QList>

#include "IO/Message.h"
#include "Config/Configuration.h"
#include "DataObjects/GriddedData.h"
#include "VTD/GBVTD.h"
#include "DataObjects/Coefficient.h"
#include "DataObjects/SimplexList.h"
//#include "DataObjects/Center.h"

class SimplexThread : public QThread
{
  Q_OBJECT
    
 public:
     SimplexThread(QObject *parent = 0);
     ~SimplexThread();
	 void findCenter(Configuration *wholeConfig, GriddedData *dataPtr, 
			 float *vortexLat, float *vortexLon);
	 

	 //bool chooseCenter();
	 //bool initialize();
	 
 public slots:
     void catchLog(const Message& message);
   
 protected:
     void run();
 
 signals:
     void centerFound();
     void log(const Message& message);
 
 private:
     QMutex mutex;
     QWaitCondition waitForData;
     bool abort;
	 GriddedData *gridData;
	 Configuration *configData;
	 QDomElement simplexConfig;
	 float* refLat;
	 float* refLon;
	 float* dataGaps;
	 GBVTD* vtd;
	 Coefficient* vtdCoeffs; 
	 float vtdStdDev;
	 float VTsolution;
	 float Xsolution;
	 float Ysolution;	 
	 float meanXall, meanYall, meanVTall;
	 float meanX, meanY, meanVT;
	 float stdDevVertexAll, stdDevVTAll;
	 float stdDevVertex, stdDevVT;
	 float convergingCenters;
	 float Xind[25],Yind[25],VTind[25];
	 float Xconv[25],Yconv[25],VTconv[25];
	 SimplexList simplexResults;
	 
	 void archiveCenters(float& radius, float& height);
	 inline void getVertexSum(float**& vertex,float*& vertexSum);
	 float simplexTest(float**& vertex, float*& VT, float*& vertexSum, 
			   float& radius, float& height, float& RefK,
			   QString& velField, int& high,double factor);
	 
	 // Choosecenter variables
	 QDomElement chooseCElem;
	 float distWeight, windWeight, stdWeight, ptsWeight;
	 float fCriteria[30];
	 QDateTime startTime, stopTime;
	 //const float velNull = -999;

};

#endif
