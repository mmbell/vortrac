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
  //Message::toScreen("AnalysisThread Constructor");
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

  numVolProcessed = 0;
  configData = NULL;
  radarVolume = NULL;
  vortexList = NULL;
  simplexList = NULL;
  pressureList = NULL;
  dropSondeList = NULL;

  gridFactory.setAbort(&abort);
}

AnalysisThread::~AnalysisThread()
{
  Message::toScreen("AnalysisThread Destructor IN");
  abort = true;

  if(this->isRunning())
    this->abortThread();

  this->exit();

  // Delete Members
  //delete simplexThread;
  configData = NULL;
  delete configData;
  radarVolume = NULL;
  delete radarVolume;
  vortexList = NULL;
  delete vortexList;
  simplexList = NULL;
  delete simplexList;
  pressureList = NULL;
  delete pressureList;
  dropSondeList = NULL;
  delete dropSondeList;

  Message::toScreen("AnalysisThread Destructor OUT");
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

void AnalysisThread::setDropSondeList(PressureList *archivePtr)
{

        dropSondeList = archivePtr;

}

void AnalysisThread::abortThread()
{
  Message::toScreen("In AnalysisThread Abort");

  delete simplexThread; // This goes first so it can break the cycle which will
  // allow us a change to lock if we are in simplexThread
  // Otherwise we could wait a while for a lock  
  delete vortexThread;    

  mutex.lock();
  abort = true;
  mutex.unlock();
  
  // Wait for the thread to finish running if it is still processing
  if(this->isRunning()) {
    Message::toScreen("This is running - analysisThread");
    waitForData.wakeOne();
    // Message::toScreen("WaitForData.wakeAll() - passed");
    wait();
    Message::toScreen("Got past wait - AnalysisThread");
    // Got rid of wait because it was getting stuck when aborted in simplex cycle
  }
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
  //  Message::toScreen("Entering AnalysisThread - Run");
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

		  if(numVolProcessed == 0) {
		    //Message::toScreen("Initializing the lists....");
		        // Need to initialize the Lists
			QDomElement radar = configData->getConfig("radar");
			QString radarName = configData->getParam(radar,"name");
			
			QDomElement vortex = configData->getConfig("vortex");
			QString vortexName = configData->getParam(vortex,"name");
			vortexLat = configData->getParam(vortex,"lat").toFloat();
			vortexLon = configData->getParam(vortex,"lon").toFloat();
			QString year;
			year.setNum(QDate::fromString(configData->getParam(radar,"startdate"), "yyyy-MM-dd").year());
			
			QString workingPath = configData->getParam(configData->getConfig("vortex"),"dir");
			QString vortexPath = configData->getParam(configData->getConfig("vtd"), "dir");
			QString outFileName = workingPath + "/"+vortexName+"_"+radarName+"_"+year+"_vortexList.xml";
			vortexList->setFileName(outFileName);
			vortexList->setRadarName(radarName);
			vortexList->setVortexName(vortexName);
			vortexList->setNewWorkingDirectory(vortexPath + "/");
			
			QString simplexPath = configData->getParam(configData->getConfig("center"),"dir");
			outFileName = workingPath + "/"+vortexName+"_"+radarName+"_"+year+"_simplexList.xml";
			simplexList->setFileName(outFileName);
			simplexList->setRadarName(radarName);
			simplexList->setVortexName(vortexName);
			simplexList->setNewWorkingDirectory(simplexPath + "/");
			
			// Put the pressure output in the workingDir for now, since the pressure obs
			// may be somewhere where we can't write
 
			outFileName = workingPath + "/"+vortexName+"_"+radarName+"_"+year+"_pressureList.xml";
			pressureList->setFileName(outFileName);
			pressureList->setRadarName(radarName);
			pressureList->setVortexName(vortexName);
			pressureList->setNewWorkingDirectory(workingPath + "/");

			// Put the dropSonde output in the workingDir for now

			outFileName = workingPath + "/"+vortexName+"_"+radarName+"_"+year+"_dropSondeList.xml";
			dropSondeList->setFileName(outFileName);
			dropSondeList->setRadarName(radarName);
			dropSondeList->setVortexName(vortexName);
			dropSondeList->setNewWorkingDirectory(workingPath + "/");
		  }
		  else {
		    // We have already attempted to process volumes which 
		    // means that the vortex was effectively out of radar
		    // range.  Get interpolated lat and lon coordinates
		    // from initial position and velocity guess
		    
		    // get start date and time from radar config
		    
		    QDomElement radar = configData->getConfig("radar");
		    QString dateString = configData->getParam(radar,
							      "startdate");
		    QString timeString = configData->getParam(radar,
							      "starttime");
		    QDate radarStartDate = QDate::fromString(dateString,
							     Qt::ISODate);
		    QTime radarStartTime = QTime::fromString(timeString,
							     Qt::ISODate);
		    QDateTime radarStartDateTime = QDateTime(radarStartDate, 
							     radarStartTime, 
							     Qt::UTC);
		    // Get direction (degrees cw from north) and speed (m/s)
		    // of initial storm position
		    
		    QDomElement vortex = configData->getConfig("vortex");
		    float stormSpeed = configData->getParam(vortex, 
							    "speed").toFloat();
		    float stormDirection = configData->getParam(vortex, 
								"direction").toFloat();
		    // Put the storm direction in math coordinates 
		    // radians ccw from east

		    stormDirection = 450-stormDirection;
		    if(stormDirection > 360)
		      stormDirection -=360;
		    stormDirection*=acos(-1)/180.;

		    // Get initial lat and lon
		    vortexLat = configData->getParam(vortex,"lat").toFloat();
		    vortexLon = configData->getParam(vortex,"lon").toFloat();

		    // Get this volume's time
		    
		    QDateTime volDateTime = radarVolume->getDateTime();
		    
		    int elapsedSeconds =radarStartDateTime.secsTo(volDateTime);
		    //Message::toScreen("Seconds since start "+QString().setNum(elapsedSeconds)+" in AnalysisThread");
		    float distanceMoved = elapsedSeconds*stormSpeed/1000.0;
		    float changeInX = distanceMoved*cos(stormDirection);
		    float changeInY = distanceMoved*sin(stormDirection);
		    float *newLatLon = GriddedData::getAdjustedLatLon(vortexLat,vortexLon, changeInX, changeInY);
		    vortexLat = newLatLon[0];
		    vortexLon = newLatLon[1];
		    //Message::toScreen("New vortexLat = "+QString().setNum(vortexLat)+" New vortexLon = "+QString().setNum(vortexLon));

		  }
		}
       
		// Check to see if the center is beyond 174 km
		// If so, tell the user to wait!
		//Message::toScreen("RLat = "+QString().setNum(*radarVolume->getRadarLat())+" RLon = "+QString().setNum(*radarVolume->getRadarLon())+" VLat = "+QString().setNum(vortexLat)+" VLon = "+QString().setNum(vortexLon));
		float relDist = GriddedData::getCartesianDistance(
				             radarVolume->getRadarLat(), 
					     radarVolume->getRadarLon(),
					     &vortexLat, &vortexLon);
		//Message::toScreen("Distance Between Radar and Storm "+QString().setNum(relDist));
		bool beyondRadar = true;
		bool closeToEdge = false;
		//for(int i = 0; i < radarVolume->getNumSweeps(); i++) {
		//if(relDist < radarVolume->getSweep(i)->getUnambig_range()){
		//    beyondRadar = false;
		if(relDist < 174) {
		  beyondRadar = false;
		  //Message::toScreen("Level "+QString().setNum(i)+" screws it up with range "+QString().setNum(radarVolume->getSweep(i)->getUnambig_range()));
		}
		// }
		//  if((relDist > radarVolume->getSweep(i)->getUnambig_range()-10)&&(relDist < radarVolume->getSweep(i)->getUnambig_range()+5))
		//    closeToEdge = true;
		//}
		if (beyondRadar) {
		  // Too far away for Doppler, need to send a signal
		  emit log(Message("Estimated center is out of Doppler range!"));
		  Message::toScreen("Estimated center is out of Doppler range!");
		  delete radarVolume;
		  emit doneProcessing();
		  waitForData.wait(&mutex);
		  mutex.unlock();
		  continue;
		}
		if (closeToEdge) {
		  //Message::toScreen("This volume is close to the edge");
		}

		//Message::toScreen("gets to create vortexData analysisThread");

		// Create data instance to hold the analysis results
		VortexData *vortexData = new VortexData(); 
		vortexData->setTime(radarVolume->getDateTime());
		
		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();
		
		// Pass VCP value to display
		emit newVCP(radarVolume->getVCP());

		// Dealias 		
		if(!radarVolume->isDealiased()){
		  
		  RadarQC *dealiaser = new RadarQC(radarVolume);

		  connect(dealiaser, SIGNAL(log(const Message&)), this, 
			  SLOT(catchLog(const Message&)), Qt::DirectConnection);
		  
		  dealiaser->getConfig(configData->getConfig("qc"));
		  
		  if(dealiaser->dealias()) {
		    emit log(Message("Finished QC and Dealiasing", 10));
		    radarVolume->isDealiased(true);
		  } else {
		    emit log(Message("Finished Dealias Method with Failures"));
		    analysisGood = false;
		    // Something went wrong
		  }
		  delete dealiaser;
		}
		else
		  emit log(Message("RadarVolume is Dealiased"));
		         /*
		
			 // Using this for running FORTRAN version
			 QString name("fchar0007.dat");
			 Message::toScreen("Writing Vortrac Input "+name);
			 radarVolume->writeToFile(name);
			 Message::toScreen("Wrote Vortrac Input "+name);
			 */
		
		         /*
			 // Testing HVVP before Cappi to save time 
			 // only good for Charley 1824
			 
			 float rt1 = 167.928;
			 float cca1 = 177.204;
			 float rmw1 = 11;
			 
			 // Testing HVVP before Cappi to save time 
			 // only good for Charley 140007
			 
			 float rt1 = 87.7712;
			 float cca1 = 60.1703;
			 float rmw1 = 16.667;
			 
			 
			 // Testing HVVP before Cappi to save time 
			 // only good for Katrina 0933
			 
			 float rt1 = 164.892;
			 float cca1 = 172.037;
			 float rmw1 = 31;
			 
			 
			 // Testing HVVP before Cappi to save time 
			 // only good for Katrina 1044
			 
			 float rt1 = 131.505;
			 float cca1 = 168.458;
			 float rmw1 = 25;
		
			 
			 // Testing HVVP before Cappi to save time 
			 // only good for Analytic Charley
			 
			 float rt1 = 70.60;
			 float cca1 = 45.19;
			 float rmw1 = 7;
			 
			 Message::toScreen("Hvvp Parameters: Distance to Radar "+QString().setNum(rt1)+" angel to vortex center in degrees ccw from north "+QString().setNum(cca1)+" rmw "+QString().setNum(rmw1));
			 
			 Hvvp *envWindFinder1 = new Hvvp;
			 envWindFinder1->setRadarData(radarVolume,rt1, cca1, rmw1);
			 envWindFinder1->findHVVPWinds();
			 float missingLink1 = envWindFinder1->getAvAcrossBeamWinds();
			 Message::toScreen("Hvvp gives "+QString().setNum(missingLink1));
			 delete envWindFinder1;
			 
			 //return;
			 */
 
		mutex.unlock();
		if(abort)
		  return;

		/* mutex.lock(); 
		 *
		 * We have to leave this section unlocked so that a change 
		 * in abort can occur and be sent through the chain to 
		 * cappi routines, otherwise the thread cannot exit until 
		 * they return -LM
		 */
		
		// Create CAPPI
		emit log(Message("Creating CAPPI..."));
		
		/*  If Analytic Model is running we need to make an analytic
		 *  gridded data rather than a cappi
		 */
		GriddedData *gridData;
			
		if(radarVolume->getNumSweeps() < 0) {
		  Configuration *analyticConfig = new Configuration();
		  QDomElement radar = configData->getConfig("radar");
		  float radarLat = configData->getParam(radar,"lat").toFloat();
		  float radarLon = configData->getParam(radar,"lon").toFloat();
		  analyticConfig->read(configData->getParam(radar, "dir"));
		  gridData = gridFactory.makeAnalytic(radarVolume,
					   configData,analyticConfig, 
					   &vortexLat, &vortexLon, 
					   &radarLat, &radarLon);
		}
		else {
		  gridData = gridFactory.makeCappi(radarVolume, configData,
						   &vortexLat, &vortexLon);
		  //Message::toScreen("AnalysisThread: outside makeCappi");
		  
		}
		
		if(abort)
		  return;
		
		// Pass Cappi to display
		emit newCappi(gridData);
		
		//mutex.unlock();
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
		
		
	      
		//Find Center 
		simplexThread->findCenter(configData, gridData, simplexList, vortexData);
		waitForCenter.wait(&mutex); 
					 
		QString simplexTime;
		simplexTime.setNum((float)analysisTime.elapsed() / 60000);
		simplexTime.append(" minutes elapsed");
		emit log(Message(simplexTime));

		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();

		//vortexData->printString(); //Prints new vortexData
	
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
		float rmw = 0;
		int goodrmw = 0;
		for(int level = 0; level < vortexData->getNumLevels(); level++) {
		  if(vortexData->getRMW(level)!=-999) {
		    //Message::toScreen("radius @ level "+QString().setNum(level)+" = "+QString().setNum(vortexData->getRMW(level)));
		    rmw += vortexData->getRMW(level);
		    goodrmw++;
		  }
		}
		rmw = rmw/(1.0*goodrmw);
	    
       // RMW is the average rmw taken over all levels of the vortexData
       // The multiplying by 2 bit is blatant cheating, I don't know 
       // if this if the radius's returned need to be adjusted by spacing or
       // or what the deal is but these numbers look closer to right for 
       // the two volumes I am looking at.

		Message::toScreen("Hvvp Parameters: Distance to Radar "+QString().setNum(rt)+" angel to vortex center in degrees ccw from north "+QString().setNum(cca)+" rmw "+QString().setNum(rmw));

		Hvvp *envWindFinder = new Hvvp;
		envWindFinder->setRadarData(radarVolume,rt, cca, rmw);
		//envWindFinder->findHVVPWinds(false); for first fit only
		envWindFinder->findHVVPWinds(true);
		float missingLink = envWindFinder->getAvAcrossBeamWinds();
		Message::toScreen("Hvvp gives "+QString().setNum(missingLink));

		mutex.unlock();  	
		
		if (!abort) {
		  
		  mutex.lock();
		  // Get the GBVTD winds
		  vortexThread->getWinds(configData, gridData, 
					 vortexData, pressureList);
		  waitForWinds.wait(&mutex); 
		  mutex.unlock();
		}
		
		if(abort)
		  return;  
				 
		// Should have all relevant variables now
		// Update timeline and output


		// Check to see if the vortex data is valid
		// Threshold on std deviation of parameters in
		// conjuction with distance or something

		mutex.lock();

		/*
		  bool dataIsCrap = false;
		  Message::toScreen("Check to See if beyond Radar!!!!!!!");
		  if(beyondRadar){
		  dataIsCrap = true;
		  Message::toScreen("Data is crap ... not using it");
		  }
		  
		  if(closeToEdge) {
		  // Collect data about close to edge volumes for
		  // Micheal to look at
		  Message::toScreen("Collecting data for Micheal");
		  
	     
		  }
		
		if(!dataIsCrap){
		  vortexList->append(*vortexData);		
		}
		*/
		vortexList->append(*vortexData);	
		vortexList->save();
		
		// Delete CAPPI, RadarData and HVVP objects
		delete envWindFinder;
		delete radarVolume;
		delete gridData;
		vortexData = NULL;
		delete vortexData;
		
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
		if (!abort) {
		  mutex.lock();
		  // Wait until new data is available
		  waitForData.wait(&mutex);
		  mutex.unlock();
		}
		
		if(abort)
		  return;
		
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

void AnalysisThread::setNumVolProcessed(const float& num)
{
  numVolProcessed = num;
}
