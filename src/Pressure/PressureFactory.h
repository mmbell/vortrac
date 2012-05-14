/*
 *  PressureFactory.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/24/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */


#ifndef PRESSUREFACTORY_H
#define PRESSUREFACTORY_H

#include <QString>
#include <QDir>
#include <QDomElement>
#include <QQueue>
#include <QHash>
#include "Config/Configuration.h"
#include "Pressure/PressureList.h"
#include "Pressure/HWind.h"
#include "Pressure/AWIPS.h"
#include "IO/Message.h"
#include "GUI/ConfigTree.h"

class PressureFactory : public QObject
{

    Q_OBJECT

public:
    PressureFactory(Configuration *wholeConfig, QObject *parent = 0);
    ~PressureFactory();
    QList<PressureData>* getUnprocessedData();
    bool hasUnprocessedData();

public slots:
    void catchLog(const Message& message);

signals:
    void log(const Message& message);

private:
    enum dataFormat {
        hwind,
        awips,
        netcdf
    };

    QDir dataPath;
    dataFormat pressureFormat;
    QQueue<QString> *pressureQueue;
    QDateTime startDateTime;
    QDateTime endDateTime;
    QHash<QString, bool> fileParsed;
    float radarlat, radarlon;

};


#endif

