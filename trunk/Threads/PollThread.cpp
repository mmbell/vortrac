/*
 *  PollThread.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/25/05.
 *  Copyright 2005 University Corporation for Atmospheric Research. All rights reserved.
 *
 */

#include <QtGui>
#include "PollThread.h"
#include "Message.h"

PollThread::PollThread(QObject *parent)
  : QThread(parent)
{
 
  abort = false;
  connect(&analysisThread, SIGNAL(doneProcessing()), 
  	  this, SLOT(analysisDoneProcessing()));
  connect(&analysisThread, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  connect(this, SIGNAL(terminated()),
	  &analysisThread, SLOT(terminate()));
 
}

PollThread::~PollThread()
{
  mutex.lock();
  abort = true;
  waitForAnalysis.wakeOne();
  mutex.unlock();
  
  wait();
}

void PollThread::setConfig(Configuration *configPtr)
{

  configData = configPtr;

}

void PollThread::analysisDoneProcessing()
{
  waitForAnalysis.wakeOne();
}

void PollThread::run()
{
  emit log(Message("PollThread Started"));
  RadarFactory *dataSource = new RadarFactory(configData->getConfig("radar"));
  connect(dataSource, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);

  vortexList = new QList<VortexData>;
  analysisThread.setVortexList(vortexList);

	// Begin polling loop
	forever {

		// Check for new data
		if (dataSource->hasUnprocessedData()) {
			// Got some data, fire up the thread to process it
			analysisThread.analyze(dataSource->getUnprocessedData(),
					       configData);
			mutex.lock();
			if (!abort) {
			  waitForAnalysis.wait(&mutex);
			}
			mutex.unlock();  
		}
		
		// Check to see if we should quit
		if (abort)
			return;
    
	}

	emit log(Message("PollThread Finished"));	
}

// This slot is used for log message relaying
// Any objects created by this object must be connected
// to this slot

void PollThread::catchLog(const Message& message)
{
  emit log(message);
}


