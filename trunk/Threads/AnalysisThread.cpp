/*
 *  AnalysisThread.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 6/17/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <QtGui>

#include "AnalysisThread.h"
#include "Radar/RadarQC.h"
#include "DataObjects/CappiGrid.h"
#include "DataObjects/VortexData.h"
#include "Message.h"
#include "SimplexThread.h"

AnalysisThread::AnalysisThread(QObject *parent)
  : QThread(parent)
{

  abort = false;
  connect(&simplexThread, SIGNAL(log(const Message&)), this, 
			SLOT(catchLog(const Message&)), Qt::DirectConnection);
  connect(&simplexThread, SIGNAL(centerFound()), this,
		  SLOT(foundCenter()));
}

AnalysisThread::~AnalysisThread()
{
  mutex.lock();
  abort = true;
  waitForData.wakeOne();
  mutex.unlock();
  
  // Wait for the thread to finish running if it is still processing
  wait();

}

void AnalysisThread::setConfig(Configuration *configPtr)
{

  configData = configPtr;

}

void AnalysisThread::setVortexList(VortexList *archivePtr)
{

  vortexList = archivePtr;

}

void AnalysisThread::analyze(RadarData *dataVolume, Configuration *configPtr)
{
	// Lock the thread
	QMutexLocker locker(&mutex);

	this->radarVolume = dataVolume;
	configData = configPtr;

	// Start or wake the thread
	if(!isRunning()) {
		start();
	} else {
		waitForData.wakeOne();
	}
}

void AnalysisThread::run()
{
	   emit log(Message("Data found, starting analysis..."));

       forever {
		// Check to see if we should quit
		if (abort)
		  return;

		// OK, Let's process some radar data
		mutex.lock();
		bool analysisGood = true;
		
		// Read in the radar data
		radarVolume->readVolume();

		// Pass VCP value to display
		emit newVCP(radarVolume->getVCP());
	       
		//Dealias
		if(!radarVolume->isDealiased()){
		  
		  RadarQC dealiaser(radarVolume);

		  connect(&dealiaser, SIGNAL(log(const Message&)), this, 
			  SLOT(catchLog(const Message&)), Qt::DirectConnection);
		  
		  dealiaser.getConfig(configData->getConfig("qc"));
		  
		  if(dealiaser.dealias()) {
		    emit log(Message("Finished Dealias Method", 10));
		    radarVolume->isDealiased(true);
		  } else {
		    emit log(Message("Finished Dealias Method with Failures"));
		    analysisGood = false;
		    // Something went wrong
		  }
		}
		else
		  emit log(Message("RadarVolume is Dealiased"));
		  
		// Check the vortex list for a current center
		if (!vortexList->isEmpty()) {
		  vortexList->timeSort();
		  vortexLat = vortexList->last().getLat();
		  vortexLon = vortexList->last().getLon();
		} else {
		  vortexLat = configData->getConfig("vortex").firstChildElement("lat").text().toFloat();
		  vortexLon = configData->getConfig("vortex").firstChildElement("lon").text().toFloat();
		}
		
		// Create CAPPI
		emit log(Message("Creating CAPPI..."));

		/* If Analytic Model is running we need to make an analytic
		   gridded data rather than a cappi*/
		
		GriddedData *gridData;

		if(radarVolume->getNumSweeps() < 0) {
		  Configuration *analyticConfig = new Configuration();
		  QDomElement radar = configData->getConfig("radar");
		  float radarLat = configData->getParam(radar,"lat").toFloat();
		  float radarLon = configData->getParam(radar,"lon").toFloat();
		  analyticConfig->read(configData->getParam(radar, "dir"));
		  gridData = gridFactory.makeAnalytic(radarVolume,
					   configData->getConfig("cappi"),
					   analyticConfig, &vortexLat, 
					   &vortexLon, &radarLat, &radarLon);
		}
		else {
		  gridData = gridFactory.makeCappi(radarVolume,
						configData->getConfig("cappi"),
			 			&vortexLat, &vortexLon);

		}
		
		
		/* GriddedData not currently a QObject so this will fail
		   connect(gridData, SIGNAL(log(const Message&)),this,
		   SLOT(catchLog(const Message&)), Qt::DirectConnection); */
		
		// Output Radar data to check if dealias worked
		gridData->writeAsi();
		emit log(Message("Writing ASI"));
		
		// Create vortexdata instance to hold the analysis results
		VortexData *vortexdata = new VortexData(); 
		
		// Find Center
		simplexThread.findCenter(configData, gridData, &vortexLat, &vortexLon);

		mutex.lock();
		if (!abort) {
			waitForCenter.wait(&mutex);
		}
		mutex.unlock();  
		
/*
		
		// Get environmental wind
		vortexdata->calculateEnvWind(configdata.getEnvWindParams());
		
		// Run core VTD algorithm
		vortexdata->runVTD(configdata.getVTDParams());
		
		// Get current pressure values
		PressureData *pressuredata = new PressureData;
		pressuredata->setPressure(datasource->getPressure());
		
		// Calculate central pressure
		vortexdata->setCentralPressure(pressuredata->getPressure());
		
		// Should have all relevant variables now
		// Update timeline and output
		vortexlist->addVortex(vortexdata->getArchiveData());
		
		*/
		delete vortexdata;
		
		if(!analysisGood)
		{
			// Some error occurred, notify the user
			emit log(Message("Radar volume processing error!"));
			emit log(Message("Radar volume processing error!"));
			return;
		} else {
			// Store the resulting analysis in the vortex list
			archiveAnalysis();
			
			// Complete the progress bar and log that we're done
			emit log(Message("Analysis complete!",100));

			// Let the poller know we're done
			emit(doneProcessing());
		}
		mutex.unlock();
		
		// Go to sleep, wait for more data
		mutex.lock();
		if (!abort)
			// Wait until new data is available
			waitForData.wait(&mutex);
		mutex.unlock();
		
	}
	emit log(Message("End of Analysis Thread Run"));
}

void AnalysisThread::archiveAnalysis()
{

}

void AnalysisThread::foundCenter()
{
	waitForCenter.wakeOne();
}

void AnalysisThread::catchLog(const Message& message)
{
  emit log(message);
}
