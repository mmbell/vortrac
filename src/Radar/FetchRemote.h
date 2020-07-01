/*
 * FetchRemote.h
 * VORTRAC
 *
 * Created by Annette Foerster 2012
 * Copyright 2012 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 *
 */

#ifndef FETCHREMOTE_H
#define FETCHREMOTE_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>
#include <QObject>
#include "Config/Configuration.h"
#include "IO/Message.h"
#include "Config/thredds_Config.h"

class FetchRemote : public QObject
{
    Q_OBJECT

public:
    FetchRemote(Configuration* config, QObject *parent = 0);
    ~FetchRemote();
    void setConfiguration(Configuration* config);

public slots:
    void catchLog(const Message& message);

signals:
    void log(const Message& message);

private:
    Configuration *configData;

private slots:
    void fetchRemoteData();
    bool getRemoteData();
    bool saveRemoteData(QNetworkReply *reply);

private:
    QNetworkAccessManager catalog_manager;
    QNetworkAccessManager datafile_manager;
    QNetworkReply* catalog_reply;
    QList<QNetworkReply *> datafile_replies;
    QStringList urlList;
};

#endif // FETCHREMOTE_H
