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

#include <QSize>
#include <QList>
#include <QObject>

#include "IO/Message.h"
#include "Config/Configuration.h"
#include "DataObjects/GriddedData.h"
#include "VTD/GBVTD.h"
#include "DataObjects/Coefficient.h"
#include "DataObjects/SimplexList.h"
#include "DataObjects/SimplexData.h"
#include "DataObjects/VortexData.h"


class SimplexThread:public QObject
{
    Q_OBJECT
    
public:
    SimplexThread(QObject* parent=0);
    ~SimplexThread();
    void initParam(Configuration *wholeConfig, GriddedData *dataPtr,float latGuess, float lonGuess);
    bool findCenter(SimplexList* simplexList);

public slots:
    void catchLog(const Message& message);

signals:
    void log(const Message& message);

private:
    GriddedData   *gridData;
    Configuration *configData;
    float _latGuess;
    float _lonGuess;
    float* _dataGaps;
    GBVTD* _simplexVTD;
    Coefficient* _vtdCoeffs;
    float* VT;
    float* vertexSum;
    float firstLevel;
    float lastLevel;
    float firstRing;
    float lastRing;
    float vtdStdDev;
    float meanXall, meanYall, meanVTall;
    float meanX, meanY, meanVT;
    float stdDevVertexAll, stdDevVTAll;
    float stdDevVertex, stdDevVT;
    float convergingCenters;
    float endX[25],endY[25],VTind[25];
    float Xconv[25],Yconv[25],VTconv[25];
    float startX[25], startY[25];


    void archiveCenters(SimplexData* simplexData,float radius,float height,float numPoints);
    void archiveNull(SimplexData* simplexData,float& radius,float& height,float& numPoints);
    inline void _getVertexSum(float** vertex,float* vertexSum);
    float _simplexTest(float**& vertex, float*& VT, float*& vertexSum,
                      float& radius, float& height, float& RefK,
                      QString& velField, int& high,double factor);

    // Choosecenter variables
    float velNull;
    float _getSymWind(float vertex_x,float vertex_y,int RefK,float radius,float height,QString velField);
    void  _centerIterate(float** vertex,float* vertexSum, float* VT,int maxIterations,float convergeCriterion,
                          float RefK,float radius,float height,QString velField,float& VTsolution,float& Xsolution,float& Ysolution);
};

#endif
