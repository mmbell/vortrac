/*
 * Driver.h
 * VORTRAC
 *
 * Created by Annette Foerster in 2012
 * Copyright 2005 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 *
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <QTextEdit>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "Config/Configuration.h"
#include "IO/Log.h"
#include "ConfigTree.h"
#include "Threads/workThread.h"
//#include "GraphFace.h"
//#include "ConfigurationDialog.h"
//#include "DiagnosticPanel.h"
//#include "CappiDisplay.h"
//#include "StartDialog.h"
#include "IO/ATCF.h"
#include "Pressure/MADISFactory.h"
#include "Radar/FetchRemote.h"
//#include "TestGraph.h"

#include <QString>
#include <QDomDocument>


class Driver
{

public:
    Driver();
    ~Driver();
    virtual bool initialize(const QDomElement& configuration) = 0;
    virtual bool run() = 0;
    virtual bool finalize() = 0;

protected:
    QDomDocument domDoc;
    bool parseXMLconfig(const QDomElement& config);
    bool validateXMLconfig();

};

#endif // DRIVER_H
