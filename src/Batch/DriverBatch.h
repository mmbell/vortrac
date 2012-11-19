/*
 * DriverBatch.h
 * VORTRAC
 *
 * Created by Annette Foerster in 2012
 * Copyright 2005 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 *
 */


#ifndef DRIVERBATCH_H
#define DRIVERBATCH_H

#include <QWidget>
#include <QTextEdit>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDomDocument>

#include "Config/Configuration.h"
#include "Threads/workThread.h"
#include "IO/Log.h"
#include "IO/Message.h"
#include "IO/ATCF.h"
#include "Pressure/MADISFactory.h"
#include "Radar/FetchRemote.h"

class DriverBatch  : public QWidget
{
    Q_OBJECT

public:
    DriverBatch(QWidget *parent, QDomDocument& xmlfile);
    ~DriverBatch();
    bool initialize();
    bool run();
    bool finalize();

public slots:
    void saveLog();
    void catchLog(const Message& message);

signals:
    void log(const Message& message);

protected:
    workThread *pollThread;
    QDomDocument *domDoc;
    bool parseXMLconfig();
    bool validateXMLconfig();

private:
    QString vortexLabel;
    QString configFileName;
    Configuration *configData;
    Log *statusLog;
    QTextEdit *statusText;

    ATCF *atcf;
    MADISFactory *madis;
    FetchRemote *fetchremote;

};

#endif // DRIVERBATCH_H
