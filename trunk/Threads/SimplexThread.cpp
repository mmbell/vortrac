/*
 *  SimplexThread.cpp
 *  vortrac
 *
 *  Created by Michael Bell on 2/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <QtGui>

#include "SimplexThread.h"

SimplexThread::SimplexThread(QObject *parent)
  : QThread(parent)
{
  abort = false;
}

SimplexThread::~SimplexThread()
{
  mutex.lock();
  abort = true;
  waitForData.wakeOne();
  mutex.unlock();
  
  // Wait for the thread to finish running if it is still processing
  wait();

}

void SimplexThread::findCenter(QDomElement centerConfig, GriddedData *dataPtr)
{

	// Lock the thread
	QMutexLocker locker(&mutex);

	gridData = dataPtr;

	// Start or wake the thread
	if(!isRunning()) {
		start();
	} else {
		waitForData.wakeOne();
	}
}

void SimplexThread::run()
{
	emit log(Message("SimplexThread Started"));
  
	forever {
		// Check to see if we should quit
		if (abort)
		  return;

		// OK, Let's find a center
		mutex.lock();
		bool foundCenter = true;
		
		

		if(!foundCenter)
		{
			// Some error occurred, notify the user
			emit log(Message("Simplex Error!"));
			return;
		} else {
			// Return the simplex results
			archiveCenters();
		
			// Update the progress bar and log
			emit log(Message("Found center!",60));

			// Let the poller know we're done
			emit(centerFound());
		}
		mutex.unlock();
		
		// Go to sleep, wait for more data
		mutex.lock();
		if (!abort)
		{
			// Wait until new data is available
			waitForData.wait(&mutex);
			mutex.unlock();		
		}
		
		emit log(Message("End of Simplex Run"));
	}
}

void SimplexThread::archiveCenters()
{

	// Do something with the centers
	
}

void SimplexThread::catchLog(const Message& message)
{
  emit log(message);
}
