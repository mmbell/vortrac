/*
 *  SimplexThread.h
 *  vortrac
 *
 *  Created by Michael Bell on 2/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef SIMPLEXTHREAD_H
#define SIMPLEXTHREAD_H

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>
#include <QList>

#include "IO/Message.h"
#include "Config/Configuration.h"
#include "DataObjects/GriddedData.h"

class SimplexThread : public QThread
{
  Q_OBJECT
    
 public:
     SimplexThread(QObject *parent = 0);
     ~SimplexThread();
	 void findCenter(QDomElement centerConfig, GriddedData *dataPtr);
	 
 public slots:
     void catchLog(const Message& message);
   
 protected:
     void run();
 
 signals:
     void centerFound();
     void log(const Message& message);
 
 private:
     QMutex mutex;
     QWaitCondition waitForData;
     bool abort;
	 GriddedData *gridData;
	 void archiveCenters();

};

#endif
