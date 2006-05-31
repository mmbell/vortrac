/*
 *  PollThread.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/25/05.
 *  Copyright 2005 University Corporation for Atmospheric Research. 
 *	All rights reserved.
 *
 */

#ifndef POLLTHREAD_H
#define POLLTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QList>
#include "Radar/RadarFactory.h"
#include "AnalysisThread.h"
#include "Config/Configuration.h"
#include "DataObjects/VortexList.h"

#include <QTextStream>

class PollThread : public QThread
{
  Q_OBJECT

  public:
	PollThread(QObject *parent = 0);
	~PollThread();
	void setConfig(Configuration *configPtr);
	
  public slots:
        void catchLog(const Message& message);
        void analysisDoneProcessing();
	void catchVCP(const int vcp);

  signals:
	void log(const Message& message);
	void newVCP(const int);
	
  protected:
	void run();

  private:
	QMutex mutex;
	QWaitCondition waitForAnalysis;
	bool abort;
	RadarFactory *dataSource;
	Configuration *configData;
	VortexList *vortexList;
	AnalysisThread analysisThread;

	Configuration *vortexConfig;

};

#endif
