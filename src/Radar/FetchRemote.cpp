/*
 * FetchRemote.cpp
 * VORTRAC
 *
 * Created by Annette Foerster 2012
 * Copyright 2012 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 *
 */

#include "FetchRemote.h"
#include <QtNetwork>

FetchRemote::FetchRemote(Configuration* config, QObject *parent) : QObject(parent)
{
    this->setObjectName("FetchRemote");
    configData = config;
    connect(&datafile_manager, SIGNAL(finished(QNetworkReply*)),
            this,SLOT(saveRemoteData(QNetworkReply*)));
}

FetchRemote::~FetchRemote()
{
}

void FetchRemote::setConfiguration(Configuration *config)
{
    configData = config;
}

void FetchRemote::catchLog(const Message& message)
{
    emit log(message);
}


void FetchRemote::fetchRemoteData()
{
    // Fetch the catalog of files
    QDomElement vortex = configData->getConfig("vortex");
    QString server = configData->getParam(vortex,"level2url");
    QString catalogurl = "thredds/catalog/nexrad/level2/";
    QDomElement radar = configData->getConfig("radar");
    QString radarName = configData->getParam(radar,"name");
    QString currdate = QDateTime::currentDateTimeUtc().date().toString("yyyyMMdd");
    QUrl catalog = QUrl(server + catalogurl + radarName + "/" + currdate + "/catalog.xml");
    //QString url = catalog.toString();
    QNetworkRequest request(catalog);
    catalog_reply = catalog_manager.get(request);
    // Connect the signals and slots
    connect(catalog_reply, SIGNAL(finished()), this,
            SLOT(getRemoteData()));
}

bool FetchRemote::getRemoteData()
{
    QUrl url = catalog_reply->url();
    if (catalog_reply->error()) {
        emit log(Message(QString("Problem downloading Remote Level II catalog"),0,this->objectName(),Yellow,QString("Problem with Remote Data")));
        return false;
    }

    emit log(Message(QString("Remote Level II catalog updated"),0,this->objectName()));

    // Save then parse the file
    QDomElement radar = configData->getConfig("radar");
    QString path = configData->getParam(radar,"dir");
    QDir dataPath(path);
    QString filename(dataPath.absolutePath() + "/catalog.xml");
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit log(Message(QString("Problem saving Remote Level II catalog"),0,this->objectName(),Yellow,QString("Problem with Remote Data")));
        return false;
    }
    file.write(catalog_reply->readAll());
    file.close();
    catalog_reply->deleteLater();

    if (!file.open(QIODevice::ReadOnly |  QIODevice::Text)) {
        emit log(Message(QString("Error Opening Configuration File, Check Permissions on "+filename),0,this->objectName(),Red, QString("Check File Permissions")));
        return false;
    }

    // Clear the urlList
    urlList.clear();

    QTextStream thredds(&file);
    while (!thredds.atEnd() and (urlList.size() <= 5)) {
        QString line = thredds.readLine();
        if (line.contains("urlPath") and !line.contains("latest")) {
            QString url = line.right(60);
            url.chop(2);
            urlList << url;
        }
    }
    file.close();

    // Check to see if this file is already in the directory, or download it
    //for (int i = 0; i < urlList.size(); ++i) {
    QDomElement vortex = configData->getConfig("vortex");
    for (int i = urlList.size()-1; i >= 0; --i) {
        QUrl url = urlList.at(i);
        QString remotepath = url.path();
        QString localfile = QFileInfo(remotepath).fileName();
        if (!dataPath.exists(localfile)) {
            QString server = configData->getParam(vortex,"level2url");
            QString dataurl= "thredds/fileServer/";
            QString url = server + dataurl + urlList.at(i);
            QUrl fileurl = QUrl(url);
            QNetworkRequest request(fileurl);
            QNetworkReply *reply = datafile_manager.get(request);
            datafile_replies.append(reply);
            QString msg = "Downloading Level II data file " + localfile;
            emit log(Message(msg,0,this->objectName()));
            urlList.removeAt(i);
            break;
        }
    }

    return true;
}

bool FetchRemote::saveRemoteData(QNetworkReply *reply)
{
    // Process the first file
    bool status = false;
    QUrl url = reply->url();
    QString remotepath = url.path();
    QString filename = QFileInfo(remotepath).fileName();
    if (reply->error()) {
        QString msg = "Level II data file " + filename + " download failed: " + reply->errorString();
        emit log(Message(msg,0,this->objectName()));
        datafile_replies.removeAll(reply);
        reply->deleteLater();
    } else {
        QDomElement radar = configData->getConfig("radar");
        QString localpath = configData->getParam(radar,"dir");
        QDir dataPath(localpath);
        QFile file(dataPath.absolutePath() + "/" + filename);
        if (!file.open(QIODevice::WriteOnly)) {
            emit log(Message(QString("Problem saving remote data"),0,this->objectName(),Yellow,QString("Problem with remote data")));
        }

        file.write(reply->readAll());
        file.close();
        datafile_replies.removeAll(reply);
        reply->deleteLater();
        QString msg = "Level II data file " + filename + " downloaded successfully";
        emit log(Message(msg,0,this->objectName()));
        status = true;
    }

    // See if there are other files left
    for (int i = urlList.size()-1; i >= 0; --i) {
        QUrl url = urlList.at(i);
        QString remotepath = url.path();
        QString localfile = QFileInfo(remotepath).fileName();
        QDomElement radar = configData->getConfig("radar");
        QString path = configData->getParam(radar,"dir");
        QDir dataPath(path);
        if (!dataPath.exists(localfile)) {
            QDomElement vortex = configData->getConfig("vortex");
            QString server = configData->getParam(vortex,"level2url");
            QString dataurl= "thredds/fileServer/";
            QString url = server + dataurl + urlList.at(i);;
            QUrl fileurl = QUrl(url);
            QNetworkRequest request(fileurl);
            QNetworkReply *reply = datafile_manager.get(request);
            datafile_replies.append(reply);
            QString msg = "Downloading Level II data file " + localfile;
            emit log(Message(msg,0,this->objectName()));
            urlList.removeAt(i);
            break;
        }
    }
    return status;
}
