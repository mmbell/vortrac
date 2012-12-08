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

#include <QtGui>
#include <QWidget>
#include <QTextEdit>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDomDocument>

#include "Config/Configuration.h"
#include "GUI/ConfigurationDialog.h"
#include "Threads/workThread.h"
#include "IO/Log.h"
#include "IO/Message.h"
#include "IO/ATCF.h"
#include "Pressure/MADISFactory.h"
#include "Radar/FetchRemote.h"
#include "CappiDisplay.h"
#include "DiagnosticPanel.h"


#include "DriverAnalysis.h"

class DriverBatch  : public DriverAnalysis
{
    Q_OBJECT

public:
    DriverBatch(QWidget *parent, const QString &fileName);
    ~DriverBatch();
    bool loadFile(const QString &fileName);
    bool initialize();
    bool run();
    bool finalize();

private slots:
    void updateTcvitals();

protected:
    QString xmlfile;
    bool parseXMLconfig();
    bool validateXMLconfig();
};

#endif // DRIVERBATCH_H
