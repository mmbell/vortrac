/*
 * DriverAnalysis.h
 * VORTRAC
 *
 * Created by Annette Foerster in 2012
 * Copyright 2005 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 *
 */

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
#include "GraphFace.h"
#include "DiagnosticPanel.h"

#ifndef DRIVERANALYSIS_H
#define DRIVERANALYSIS_H

class DriverAnalysis : public QWidget
{
    Q_OBJECT

public:
    DriverAnalysis(QWidget *parent);
    virtual bool loadFile(const QString &fileName) = 0;

public slots:
    void saveLog();
    void catchLog(const Message& message);
    void updateCappiDisplay(bool hasImage);
    void updateCappiInfo(float x, float y, float rmwEstimate, float sMin, float sMax, float vMax,
                         float userCenterLat, float userCenterLon, float centerLat, float centerLon);

//private plots:
    void pollVortexUpdate(VortexList* list);
    virtual void updateTcvitals() = 0;

signals:
    void log(const Message& message);
    void updateMadis(float userCenterLat, float userCenterLon);
    void vortexListChanged(VortexList* list);

protected:
    workThread *pollThread;
    bool isUntitled;
    QDir workingDirectory;
    QString vortexLabel;
    QString configFileName;
    Configuration *configData;
    ConfigurationDialog *configDialog;

    Log *statusLog;
    QTextEdit *statusText;

    ATCF *atcf;
    MADISFactory *madis;
    FetchRemote *fetchremote;

    QLabel *currPressure;
    QLabel *currRMW;
    QLabel *currDeficit;
    QLabel* deficitLabel;

    GraphFace* graph;
    CappiDisplay* cappiDisplay;
    QLabel *appMaxWind;
    QLabel *appMaxLabel2;
    QLabel *recMaxWind;
    QLabel *recMaxLabel2;

    QLabel *lcdCenterLat;
    QLabel *lcdCenterLon;
    QLabel *lcdUserCenterLat;
    QLabel *lcdUserCenterLon;

    DiagnosticPanel *diagPanel;


};

#endif // DRIVERANALYSIS_H
