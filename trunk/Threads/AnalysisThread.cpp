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
#include "HVVP/Hvvp.h"
#include "math.h"

AnalysisThread::AnalysisThread(QObject *parent)
  : QThread(parent)
{
  Message::toScreen("AnalysisThread Constructor");
  abort = false;
  simplexThread = new SimplexThread;
  connect(simplexThread, SIGNAL(log(const Message&)), this, 
		  SLOT(catchLog(const Message&)), Qt::DirectConnection);
  connect(simplexThread, SIGNAL(centerFound()), 
		  this, SLOT(foundCenter()), Qt::DirectConnection);
  vortexThread = new VortexThread;
  connect(vortexThread, SIGNAL(log(const Message&)), this, 
		  SLOT(catchLog(const Message&)), Qt::DirectConnection);
  connect(vortexThread, SIGNAL(windsFound()), 
		  this, SLOT(foundWinds()), Qt::DirectConnection);
  
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

void AnalysisThread::setSimplexList(SimplexList *archivePtr)
{
	
	simplexList = archivePtr;
	
}

void AnalysisThread::setPressureList(PressureList *archivePtr)
{
	
	pressureList = archivePtr;
	
}

void AnalysisThread::abortThread()
{
  Message::toScreen("In AnalysisThread Abort");
  if(simplexThread->isRunning()) {
    Message::toScreen("in AnalysisThread Exit");
    mutex.lock();
    simplexThread->exit();
    //simplexThread = new SimplexThread;
    //delete simplexThread;
    //Message::toScreen("AnalysisThread has mutex locked");
    mutex.unlock();
    //}
    // exit();
  }
  Message::toScreen("Before AnalysisThread Exit");
  this->terminate();
  //  this->exit();
  Message::toScreen("Leaving AnalysisThread Abort");
}

void AnalysisThread::analyze(RadarData *dataVolume, Configuration *configPtr)
{
	// Lock the thread
	QMutexLocker locker(&mutex);

	this->radarVolume = dataVolume;
	configData = configPtr;
	radarVolume->setAltitude(configData->getParam(configData->getConfig("radar"),"alt").toFloat()/1000.0);
	// Sets altitude of radar to radarData (km) from ConfigData (meters)

	// Start or wake the thread
	if(!isRunning()) {
		start();
	} else {
	  //Message::toScreen("found running copy of analysis... waiting....");
		waitForData.wakeOne();
	}
}

void AnalysisThread::run()
{
  abort = false;

       forever {
		// Check to see if we should quit
		if (abort)
		  return;

		// OK, Let's process some radar data
		mutex.lock();
		bool analysisGood = true;
		QTime analysisTime;
		analysisTime.start();
		emit log(Message("Data found, starting analysis...", -1));
		
		// Read in the radar data
		radarVolume->readVolume();

		mutex.unlock();
		if(abort)
			return;
		mutex.lock();
		
		// Check the vortex list for a current center		
		if (!vortexList->isEmpty()) {
			vortexList->timeSort();
			vortexLat = vortexList->last().getLat();
			vortexLon = vortexList->last().getLon();
		} else {
			vortexLat = configData->getConfig("vortex").firstChildElement("lat").text().toFloat();
			vortexLon = configData->getConfig("vortex").firstChildElement("lon").text().toFloat();
			
			// Need to initialize the Lists
			QDomElement radar = configData->getConfig("radar");
			QString radarName = configData->getParam(radar,"name");
			
			QDomElement vortex = configData->getConfig("vortex");
			QString vortexName = configData->getParam(vortex,"name");
			
			QString vortexPath = configData->getConfig("vortex").firstChildElement("dir").text();
			QString vortexFile = radarVolume->getDateTimeString();
			vortexFile.replace(QString(":"),QString("_"));
			QString outFileName = vortexPath + "/" + vortexFile + "vortexList.xml";
			vortexList->setFileName(outFileName);
			vortexList->setRadarName(radarName);
			vortexList->setVortexName(vortexName);
			vortexList->setNewWorkingDirectory(vortexPath + "/");
			
			QString simplexPath = configData->getConfig("center").firstChildElement("dir").text();
			QString simplexFile = radarVolume->getDateTimeString();
			simplexFile.replace(QString(":"),QString("_"));
			outFileName = simplexPath + "/" + simplexFile + "simplexList.xml";
			simplexList->setFileName(outFileName);
			simplexList->setRadarName(radarName);
			simplexList->setVortexName(vortexName);
			simplexList->setNewWorkingDirectory(simplexPath + "/");
			
			// Put the pressure output in the workingDir for now, since the pressure obs
			// may be somewhere where we can't write
			QString pressurePath = configData->getConfig("vortex").firstChildElement("dir").text();
			QString pressureFile = radarVolume->getDateTimeString();
			pressureFile.replace(QString(":"),QString("_"));
			outFileName = pressurePath + "/" + pressureFile + "pressureList.xml";
			pressureList->setFileName(outFileName);
			pressureList->setRadarName(radarName);
			pressureList->setVortexName(vortexName);
			pressureList->setNewWorkingDirectory(pressurePath + "/");
		}
		
		// Create data instance to hold the analysis results
		VortexData *vortexData = new VortexData(); 
		vortexData->setTime(radarVolume->getDateTime());
		
		// Pass VCP value to display
		emit newVCP(radarVolume->getVCP());
		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();
		// Dealias 
		
		if(!radarVolume->isDealiased()){
		  
		  RadarQC dealiaser(radarVolume);

		  connect(&dealiaser, SIGNAL(log(const Message&)), this, 
			  SLOT(catchLog(const Message&)), Qt::DirectConnection);
		  
		  dealiaser.getConfig(configData->getConfig("qc"));
		  
		  if(dealiaser.dealias()) {
		    emit log(Message("Finished QC and Dealiasing", 10));
		    radarVolume->isDealiased(true);
		  } else {
		    emit log(Message("Finished Dealias Method with Failures"));
		    analysisGood = false;
		    // Something went wrong
		  }
		}
		else
		  emit log(Message("RadarVolume is Dealiased"));
				
		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();

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
		  //Message::toScreen("AnalysisThread: outside makeCappi");

		}
		
		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();

		/* GriddedData not currently a QObject so this will fail
		   connect(gridData, SIGNAL(log(const Message&)),this,
		   SLOT(catchLog(const Message&)), Qt::DirectConnection); */
		
		// Output Radar data to check if dealias worked
		gridData->writeAsi();
		emit log(Message("Done with Cappi",30));
		QString cappiTime;
		cappiTime.setNum((float)analysisTime.elapsed() / 60000);
		cappiTime.append(" minutes elapsed");
		emit log(Message(cappiTime));
				 
		
		// Set the initial guess in the data object as a temporary center
		vortexData->setLat(0,vortexLat);
		vortexData->setLon(0,vortexLon);
		
		mutex.unlock();  // Added this one ... I think...

		// Mutex Investigation.....
		
		mutex.lock();
		if (!abort) {
		  //Find Center 
		  simplexThread->findCenter(configData, gridData, simplexList, vortexData);
		  waitForCenter.wait(&mutex); 
		}
		else
		  return;
		mutex.unlock();
				 
		QString simplexTime;
		simplexTime.setNum((float)analysisTime.elapsed() / 60000);
		simplexTime.append(" minutes elapsed");
		emit log(Message(simplexTime));
		//Message::toScreen("Where....");

		
		// Get environmental wind
		/*
		* rt: Range from radar to circulation center (km).
		*
		* cca: Meteorological azimuth angle of the direction
		*      to the circulation center (degrees from north).
		*
		* rmw: radius of maximum wind measured from the TC circulation
		*      center outward.
		*/
		QDomElement radar = configData->getConfig("radar");
		float radarLat = configData->getParam(radar,"lat").toFloat();
		float radarLon = configData->getParam(radar,"lon").toFloat();
		
		float* distance;
		distance = gridData->getCartesianPoint(&radarLat, &radarLon, 
						       &vortexLat, &vortexLon);
		float rt = sqrt(distance[0]*distance[0]+distance[1]*distance[1]);
		float cca = atan2(distance[0], distance[1])*180/acos(-1);
		delete[] distance;
		float rmw = 12.5;

		// RMW is just a guess for the charley case right now, 
		// we need this param before we can move on with HVVP

		Message::toScreen("Hvvp Parameters: Distance to Radar"+QString().setNum(rt)+" angel to vortex center in degrees ccw from north "+QString().setNum(cca)+" rmw "+QString().setNum(rmw));

		Hvvp *envWindFinder = new Hvvp;
		envWindFinder->setRadarData(radarVolume,rt, cca, rmw);
		envWindFinder->findHVVPWinds();
		float missingLink = envWindFinder->getAvAcrossBeamWinds();
		Message::toScreen("Hvvp gives "+QString().setNum(missingLink));

		//mutex.unlock();  // Added this one ... I think...
		
		// Mutex Investigation.....
		
		mutex.lock();
		if (!abort) {
			// Get the GBVTD winds
			vortexThread->getWinds(configData, gridData, vortexData);
			waitForWinds.wait(&mutex); 
		}
		else
			return;
		mutex.unlock();  
		
		/*
		// Get current pressure values
		PressureData *pressuredata = new PressureData;
		pressuredata->setPressure(datasource->getPressure());
		
		// Calculate central pressure
		vortexdata->setCentralPressure(pressuredata->getPressure());
		*/
		
		// Should have all relevant variables now
		// Update timeline and output
		vortexList->append(*vortexData);		
		vortexList->save();
		
		mutex.lock(); // Added this one....

		// delete vortexData;
		
		//Message::toScreen("Deleted vortex data.... ???");
		
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

void AnalysisThread::foundWinds()
{
	waitForWinds.wakeOne();
}

void AnalysisThread::catchLog(const Message& message)
{
  emit log(message);
}
