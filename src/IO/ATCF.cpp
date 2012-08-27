/*
 * ATCF.cpp
 * VORTRAC
 *
 * Created by Michael Bell 2012
 *  Copyright 2012 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "ATCF.h"
#include <QtNetwork>

ATCF::ATCF(Configuration* config, QObject *parent) : QObject(parent)
{
    this->setObjectName("ATCF");
    configData = config;
    connect(&tcvitals_manager, SIGNAL(finished(QNetworkReply*)),
            SLOT(saveTcvitals(QNetworkReply*)));
    stormName = "Unknown";
    obLat = obLon = obDir = obSpd = 
    obCentralPressure = obEnvPressure = obOuterRadius = obRMW = -999.0;
}

ATCF::~ATCF()
{
    
}

void ATCF::setConfiguration(Configuration *config)
{
    configData = config;
}

void ATCF::catchLog(const Message& message)
{
    emit log(message);
}

bool ATCF::getTcvitals(bool archive)
{
    QDomElement vortex = configData->getConfig("vortex");
    QString stormId = configData->getParam(vortex,"id");
    QString dataurl = "atcf/com/";
    if (archive) {
        if (stormId.contains("L")) {
            stormId.chop(1);
            dataurl += "al" + stormId + "2012-tcvitals-arch.dat";
        } else if (stormId.contains("E")) {
            stormId.chop(1);
            dataurl += "ep" + stormId + "2012-tcvitals-arch.dat";
        }
    } else {
        if (stormId.contains("L")) {
            stormId.chop(1);
            dataurl += "al" + stormId + "2012-tcvitals.dat";
        } else if (stormId.contains("E")) {
            stormId.chop(1);
            dataurl += "ep" + stormId + "2012-tcvitals.dat";
        }
    }
    QString server = configData->getParam(vortex,"atcfurl");
    QString url = server + dataurl;
    QUrl fileurl = QUrl(url);
    QNetworkRequest request(fileurl);
    tcvitals_reply = tcvitals_manager.get(request);

    return true;
}

bool ATCF::saveTcvitals(QNetworkReply *reply)
{
    QUrl url = reply->url();
    QString remotepath = url.path();
    QString filename = QFileInfo(remotepath).fileName();
    if (reply->error()) {
        QString msg = "tcvitals file download failed: " + reply->errorString();
        emit log(Message(msg,0,this->objectName()));
        reply->deleteLater();
        return false;
    } else {
        QDomElement vortex = configData->getConfig("vortex");
        QString localpath = configData->getParam(vortex,"dir");
        QDir dataPath(localpath);
        vitalsfile.setFileName(dataPath.absolutePath() + "/" + filename);
        if (!vitalsfile.open(QIODevice::WriteOnly)) {
            emit log(Message(QString("Problem saving tcvitals"),0,this->objectName(),Yellow,QString("Problem with remote data")));
            return false;
        }
        
        vitalsfile.write(reply->readAll());
        vitalsfile.close();
        reply->deleteLater();
        QString msg = "tcvitals file downloaded successfully";
        emit log(Message(msg,0,this->objectName()));
        parseTcvitals();
        return true;
    }
}

bool ATCF::parseTcvitals()
{
    if (!vitalsfile.open(QIODevice::ReadOnly)) {
        emit log(Message(QString("Problem reading tcvitals"),0,this->objectName(),Yellow,QString("Problem with remote data")));
        return false;
    }
    QDomElement vortex = configData->getConfig("vortex");
    QString stormId = configData->getParam(vortex,"id");
    QTextStream tcvitals(&vitalsfile);
    QString stormFound("Not Found");
    while (!tcvitals.atEnd()) {
        QString line = tcvitals.readLine();
        QStringList vitals = line.split(QRegExp("\\s+"));
        if (vitals.at(1) == stormId) {
            // Match!
            stormFound = stormName = vitals.at(2);
            QDate date = QDate::fromString(vitals.at(3), "yyyyMMdd");
            QTime time = QTime::fromString(vitals.at(4), "hhmm");
            obTime = QDateTime(date, time, Qt::UTC);
            QString lat = vitals.at(5);
            if (lat.endsWith("S")) {
                lat.chop(1);
                obLat = -(lat.toFloat() / 10.0);
            } else {
                lat.chop(1);
                obLat = lat.toFloat() / 10.0;
            }
            
            QString lon = vitals.at(6);
            if (lon.endsWith("W")) {
                lon.chop(1);
                obLon = -(lon.toFloat() / 10.0);
            } else {
                lat.chop(1);
                obLon = lon.toFloat() / 10.0;
            }

            obDir = vitals.at(7).toFloat();
            obSpd = vitals.at(8).toFloat() / 10.0;
            obCentralPressure = vitals.at(9).toFloat();
            obEnvPressure = vitals.at(10).toFloat();
            obOuterRadius = vitals.at(11).toFloat();
            obRMW = vitals.at(13).toFloat();
        }
    }
    vitalsfile.close();
    
    if (stormFound == "Not Found") {
        // Try the archive data
        getTcvitals(true);
        return false;
    } else {
        emit tcvitalsReady();
        return true;
    }
}

float ATCF::getLatitude(const QDateTime& time)
{
    int elapsedSeconds =obTime.secsTo(time);
    float distanceMoved = elapsedSeconds*obSpd/1000.0;
    float dir = 450.0f-obDir;
    if(dir > 360.0f)
        dir -=360.0f;
    dir*=acos(-1.0f)/180.f;
    float changeInY = distanceMoved*sin(dir);
    float LatRadians = obLat * acos(-1.0)/180.0;
    float fac_lat = 111.13209 - 0.56605 * cos(2.0 * LatRadians)
    + 0.00012 * cos(4.0 * LatRadians) - 0.000002 * cos(6.0 * LatRadians);
    return changeInY/fac_lat + obLat;
}

float ATCF::getLongitude(const QDateTime& time)
{
    int elapsedSeconds =obTime.secsTo(time);
    float distanceMoved = elapsedSeconds*obSpd/1000.0;
    float dir = 450.0f-obDir;
    if(dir > 360.0f)
        dir -=360.0f;
    dir*=acos(-1.0f)/180.f;
    float changeInX = distanceMoved*cos(dir);
        float LatRadians = obLat * acos(-1.0)/180.0;
    float fac_lon = 111.41513 * cos(LatRadians)
    - 0.09455 * cos(3.0 * LatRadians) + 0.00012 * cos(5.0 * LatRadians);
    return changeInX/fac_lon + obLon;    
}
