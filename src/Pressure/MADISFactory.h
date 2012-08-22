/*
 * MADISFactoryFactory.h
 * VORTRAC
 *
 * Created by Michael Bell on 2012
 *  Copyright 2012 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef MADISFACTORY_H
#define MADISFACTORY_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>
#include <QObject>
#include "Config/Configuration.h"
#include "IO/Message.h"

class MADISFactory : public QObject
{
    Q_OBJECT
    
public:
    MADISFactory(Configuration* config, QObject *parent = 0);
    ~MADISFactory();
    void setConfiguration(Configuration* config);
    
public slots:
    void catchLog(const Message& message);
    void setBoundingBox(float userCenterLat, float userCenterLon);
    
signals:
    void log(const Message& message);
    void pressuresReady();

private slots:
    bool getPressureObs();
    bool savePressureObs(QNetworkReply *reply);
    
private:
    Configuration *configData;
    QNetworkAccessManager madis_manager;
    QNetworkReply* madis_reply;
    QDateTime obTime;
    QString stationName, currtime;
    float obLat, obLon, obDir, obSpd;
    float obPressure, obElevation;
    QFile madisfile;
    void setBoundingBox(const QDateTime& time);
    QString latll, latur, lonll, lonur;
};

#endif