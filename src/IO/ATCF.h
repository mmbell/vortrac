/*
 * ATCF.h
 * VORTRAC
 *
 * Created by Michael Bell on 2012
 *  Copyright 2012 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef ATCF_H
#define ATCF_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>
#include <QObject>
#include "Config/Configuration.h"
#include "IO/Message.h"

class ATCF : public QObject
{
    Q_OBJECT
    
public:
    ATCF(Configuration* config, QObject *parent = 0);
    ~ATCF();
    void setConfiguration(Configuration* config);
    float getLatitude(const QDateTime& time);
    float getLongitude(const QDateTime& time);
    float getCentralPressure() { return obCentralPressure; }
    float getEnvPressure() { return obEnvPressure; }
    float getOuterRadius() { return obOuterRadius; }
    float getRMW() { return obRMW; }
    float getDirection() { return obDir; }
    float getSpeed() { return obSpd; }
    QString getStormName() { return stormName; }
    QDateTime getTime() { return obTime; }
    
public slots:
    void catchLog(const Message& message);

signals:
    void log(const Message& message);
    void tcvitalsReady();
    
private:
    Configuration *configData;
    
private slots:
    bool getTcvitals();
    bool saveTcvitals(QNetworkReply *reply);
    bool parseTcvitals();
    
private:
    QNetworkAccessManager tcvitals_manager;
    QNetworkReply* tcvitals_reply;
    QDateTime obTime;
    QString stormName;
    float obLat, obLon, obDir, obSpd;
    float obCentralPressure, obEnvPressure, obOuterRadius, obRMW;
    QFile vitalsfile;
};

#endif