/*
 *  PollThread.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/25/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *	All rights reserved.
 *
 */

#ifndef POLLTHREAD_H
#define POLLTHREAD_H

#include <QThread>
#include <QList>
#include <QMutex>
#include <QTextStream>

#include "Radar/RadarFactory.h"
#include "AnalysisThread.h"
#include "Config/Configuration.h"
#include "DataObjects/VortexList.h"
#include "DataObjects/SimplexList.h"
#include "DataObjects/CappiGrid.h"
#include "Pressure/PressureFactory.h"
#include "Pressure/PressureList.h"
#include "ChooseCenter.h"
#include "IO/ATCF.h"

class workThread : public QThread
{
    Q_OBJECT

public:
    workThread(QObject *parent = 0);
    ~workThread();
    void setConfig(Configuration *configPtr) {configData = configPtr;}
    void setATCF(ATCF *atcfPtr) {atcf = atcfPtr;}
    void stop();

public slots:
    void catchLog(const Message& message);
    void catchVCP(const int vcp);
    void catchCappi(const GriddedData* cappi);
    void catchCappiInfo(float x,float y,float rmwEstimate,float sMin,float sMax,float vMax,
                        float userLat,float userLon,float lat,float lon);
    void setOnlyRunOnce(const bool newRunOnce = true);
    void setContinuePreviousRun(const bool &decision);

signals:
    void log(const Message& message);
    void newVCP(const int);
    void vortexListUpdate(VortexList* list);
    void newCappi(const GriddedData* cappi);
    void newCappiInfo(float x,float y,float rmwEstimate,float sMin,float sMax,float vMax,
                      float userLat,float userLon,float lat,float lon);

protected:
    void run();

private:
    bool runOnce;
    volatile bool abort;
    bool continuePreviousRun;

    RadarFactory    *dataSource;
    PressureFactory *pressureSource;
    Configuration   *configData;

    VortexList   _vortexList;
    SimplexList  _simplexList;
    PressureList _pressureList;

    float _firstGuessLat;
    float _firstGuessLon;
    void _latlonFirstGuess(RadarData* radarVolume);
    void checkIntensification();
    void checkListConsistency();

    ATCF *atcf;
};

#endif
