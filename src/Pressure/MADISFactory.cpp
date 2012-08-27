/*
 * MADISFactoryFactory.cpp
 * VORTRAC
 *
 * Created by Michael Bell 2012
 *  Copyright 2012 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "MADISFactory.h"
#include <QtNetwork>

MADISFactory::MADISFactory(Configuration* config, QObject *parent) : QObject(parent)
{
    this->setObjectName("MADISFactory");
    configData = config;
    connect(&madis_manager, SIGNAL(finished(QNetworkReply*)),
            SLOT(savePressureObs(QNetworkReply*)));
    stationName = "Unknown";
    obLat = obLon = obDir = obSpd = 
    obPressure = obElevation = -999.0;
    
    QDomElement radar = configData->getConfig("radar");
    float radarLat = configData->getParam(radar,"lat").toFloat();
    float radarLon = configData->getParam(radar,"lon").toFloat();
    latll = QString().setNum(radarLat - 2.5);
    latur = QString().setNum(radarLat + 2.5);
    lonll = QString().setNum(radarLon - 2.5);
    lonur = QString().setNum(radarLon + 2.5);
}

MADISFactory::~MADISFactory()
{
    
}

void MADISFactory::setConfiguration(Configuration *config)
{
    configData = config;
}

void MADISFactory::catchLog(const Message& message)
{
    emit log(message);
}

bool MADISFactory::getPressureObs()
{
    QDomElement pressure = configData->getConfig("pressure");
    QString server = configData->getParam(pressure,"madisurl"); // https://madis-data.noaa.gov/
    currtime = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hhmm");
    QString dataurl = "madisPublic/cgi-bin/madisXmlPublicDir?rdr=&time=0&minbck=-45&minfwd=0&recwin=3&dfltrsel=1&latll=" +
    latll + "&lonll=" + lonll + "&latur=" + latur + "&lonur=" + lonur +
    "&stanam=&stasel=0&pvdrsel=0&varsel=1&qcsel=2&xml=2&csvmiss=0&nvars=P&nvars=DD&nvars=FF&nvars=ELEV&nvars=LAT&nvars=LON";

    QString url = server + dataurl;
    QUrl fileurl = QUrl(url);
    fileurl.setUserName(configData->getParam(pressure,"madisuser"));
    fileurl.setPassword(configData->getParam(pressure,"madispassword"));
    QNetworkRequest request(fileurl);
    url = fileurl.toEncoded();
    if (QSslSocket::supportsSsl()) {
        madis_reply = madis_manager.get(request);
        return true;
    } else {
        emit log(Message(QString("This system does not support SSL"),0,this->objectName(),Yellow,QString("Problem with remote data")));
        return false;
    }
}

bool MADISFactory::savePressureObs(QNetworkReply *reply)
{
    QUrl url = reply->url();
    QString remotepath = url.path();
    QString filename = currtime + "_MADIS.txt";
    if (reply->error()) {
        QString msg = "MADIS data download failed: " + reply->errorString();
        emit log(Message(msg,0,this->objectName()));
        reply->deleteLater();
        return false;
    } else {
        QDomElement pressure = configData->getConfig("pressure");
        QString localpath = configData->getParam(pressure,"dir");
        QDir dataPath(localpath);
        madisfile.setFileName(dataPath.absolutePath() + "/" + filename);
        if (!madisfile.open(QIODevice::WriteOnly)) {
            emit log(Message(QString("Problem saving MADIS data"),0,this->objectName(),Yellow,QString("Problem with remote data")));
            return false;
        }
        QXmlStreamReader reader(reply->readAll());
        QString pressureObs;
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.isStartElement()) {
                if (reader.name() == "PRE")
                    pressureObs = reader.readElementText();
            }
        }
        if (reader.hasError()) {
            emit log(Message(QString("MADIS data not well-formed"),0,this->objectName(),Yellow,QString("Problem with remote data")));
            return false;
        }

        //madisfile.write(reply->readAll());
        madisfile.write(pressureObs.toAscii().data());
        madisfile.close();
        reply->deleteLater();
        QString msg = "MADIS data downloaded successfully";
        emit log(Message(msg,0,this->objectName()));
        emit pressuresReady();
        return true;
    }
}

void MADISFactory::setBoundingBox(float userCenterLat, float userCenterLon)
{
    latll = QString().setNum(userCenterLat - 1.5);
    latur = QString().setNum(userCenterLat + 1.5);
    lonll = QString().setNum(userCenterLon - 1.5);
    lonur = QString().setNum(userCenterLon + 1.5);
}
