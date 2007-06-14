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
  this->setObjectName("analysisThread");
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
  analyticRun = false;
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
  //Message::toScreen("AnalysisThread Destructor IN");
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

  //Message::toScreen("AnalysisThread Destructor OUT");
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
  //Message::toScreen("In AnalysisThread Abort");

  delete simplexThread; // This goes first so it can break the cycle which will
  // allow us a change to lock if we are in simplexThread
  // Otherwise we could wait a while for a lock  
  delete vortexThread;    

  mutex.lock();
  abort = true;
  mutex.unlock();
  
  // Wait for the thread to finish running if it is still processing
  if(this->isRunning()) {
    //Message::toScreen("This is running - analysisThread");
    waitForData.wakeOne();
    // Message::toScreen("WaitForData.wakeAll() - passed");
    wait();
    //Message::toScreen("Got past wait - AnalysisThread");
    // Got rid of wait because it was getting stuck when aborted in simplex cycle
    if(radarVolume!=NULL) {
      RadarData *temp = radarVolume;
      radarVolume = NULL;
      delete temp;
    }
  }
  //Message::toScreen("Leaving AnalysisThread Abort");
}

void AnalysisThread::analyze(RadarData *dataVolume, Configuration *configPtr)
{
	// Lock the thread
	QMutexLocker locker(&mutex);

	this->radarVolume = dataVolume;
	configData = configPtr;
	radarVolume->setAltitude(configData->getParam(configData->getConfig("radar"),"alt").toFloat()/1000.0);
	//Message::toScreen("AnalysisThread: Getting New RadarVolume: # Rays = "+radarVolume->getDateTimeString());
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
	 
	 //Message::toScreen("AnalysisThread: Starting From the Top");
	 
	 // Check to see if we should quit
	 if (abort) 
	   return; 
	 
	 // OK, Let's process some radar data
	 mutex.lock();

	 bool analysisGood = true;
	 QTime analysisTime;
	 analysisTime.start();    // Timer used for user info only
	 emit log(Message(QString(),2,this->objectName()));

	 bool beyondRadar;
	 
	 // Check the vortexList for a current center to use
	 // as the starting point for analysis

	 if (!vortexList->isEmpty()) {
	   vortexList->timeSort();
	   vortexLat = vortexList->last().getLat();
	   vortexLon = vortexList->last().getLon();
	 } 
	 else {
	   // If the vortexList is empty than we must load 
	   // information from the configuration to help 
	   // identify our guess center.

	   // This set initializes all the data products lists
	   // which keep a history of the analysis points in the 
	   // interface and on disk.
	   
	   QDomElement radar = configData->getConfig("radar");
	   QDomElement vortex = configData->getConfig("vortex");
	   
	   if(numVolProcessed == 0) {
	     // If no volumes have been attempted then we will use 
	     // the guess center directly from the xml configuration file.
	     
	     QString radarName = configData->getParam(radar,"name");
	     QString vortexName = configData->getParam(vortex,"name");
	     
	     QString year;
	     year.setNum(QDate::fromString(configData->getParam(radar,"startdate"), "yyyy-MM-dd").year());
	     
	     // Initializing Vortex List
	     QString workingPath = configData->getParam(configData->getConfig("vortex"),"dir");
	     QString vortexPath = configData->getParam(configData->getConfig("vtd"), "dir");
	     QString outFileName = workingPath + "/"+vortexName+"_"+radarName+"_"+year+"_vortexList.xml";
	     vortexList->setFileName(outFileName);
	     vortexList->setRadarName(radarName);
	     vortexList->setVortexName(vortexName);
	     vortexList->setNewWorkingDirectory(vortexPath + "/");
	     
	     // Initializing Simplex List
	     
	     QString simplexPath = configData->getParam(configData->getConfig("center"),"dir");
	     outFileName = workingPath + "/"+vortexName+"_"+radarName+"_"+year+"_simplexList.xml";
	     simplexList->setFileName(outFileName);
	     simplexList->setRadarName(radarName);
	     simplexList->setVortexName(vortexName);
	     simplexList->setNewWorkingDirectory(simplexPath + "/");
	     
	     // Initial Pressure Observation List
	     // Put the pressure output in the workingDir 
	     
	     outFileName = workingPath + "/"+vortexName+"_"+radarName+"_"+year+"_pressureList.xml";
	     pressureList->setFileName(outFileName);
	     pressureList->setRadarName(radarName);
	     pressureList->setVortexName(vortexName);
	     pressureList->setNewWorkingDirectory(workingPath + "/");
	     
	     // Initialize DropSonde List
	     // Put the dropSonde output in the workingDir for now
	     
	     outFileName = workingPath + "/"+vortexName+"_"+radarName+"_"+year+"_dropSondeList.xml";
	     dropSondeList->setFileName(outFileName);
	     dropSondeList->setRadarName(radarName);
	     dropSondeList->setVortexName(vortexName);
	     dropSondeList->setNewWorkingDirectory(workingPath + "/");
	   }
	   
	   // Check to make sure that the radar volume is in range
	   // get start date and time from the observation date and time
	   // in the vortex panel of the configuration.
	   
	   QString dateString = configData->getParam(vortex,"obsdate");
	   QString timeString = configData->getParam(vortex,"obstime");
	   QDate obsDate = QDate::fromString(configData->getParam(vortex,"obsdate"),"yyyy-MM-dd");
	   //Message::toScreen("obs: "+obsDate.toString("yyyy-MM-dd"));
	   QTime obsTime = QTime::fromString(timeString,"hh:mm:ss");
	   //Message::toScreen("obs: "+obsTime.toString("hh:mm:ss"));
	   QDateTime obsDateTime = QDateTime(obsDate, obsTime, Qt::UTC);
	   if(!obsDateTime.isValid()){
	     emit log(Message(QString("Observation Date or Time is not of valid format! Date: yyyy-MM-dd Time: hh:mm:ss please adjust the configuration file"),0,this->objectName(),Red, QString("ObsDate or ObsTime invalid in Config")));
	   }
	   
	   // Get this volume's time
	   //Message::toScreen("obs: "+obsDateTime.toString(Qt::ISODate));
	   
	   QDateTime volDateTime = radarVolume->getDateTime();
	   //Message::toScreen("vol: "+volDateTime.toString(Qt::ISODate));
	   
	   // If the volume time is with 15 minutes of the start time
	   // for accepting radar observations then we will use the 
	   // given vortex center for our center
	   // Message::toScreen(" secs between start and this one "+QString().setNum(obsDateTime.secsTo(volDateTime)));
	   
	   if((abs(obsDateTime.secsTo(volDateTime))<15*60)
	      ||(analyticRun)) {
	     // Message::toScreen("Using lat lon in Config");
	     vortexLat = configData->getParam(vortex,"lat").toFloat();
	     vortexLon = configData->getParam(vortex,"lon").toFloat();
	   }
	   else {
	     //Message::toScreen("Using Velocity");
	     // Use the starting position in conjunction with the storm
	     // velocity to find a current guess for the vortex center
	     
	     // Get direction (degrees cw from north) and speed (m/s)
	     // of initial storm position
	     
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
		    
	     int elapsedSeconds =obsDateTime.secsTo(volDateTime);
	     //Message::toScreen("Seconds since start "+QString().setNum(elapsedSeconds)+" in AnalysisThread");
	     if(isnan(elapsedSeconds)) {
	       emit log(Message(QString("Cannont calculate time until strom is in range of radar, Please check the observation time, latitude, longitude, and storm movement parameters"),0,this->objectName(),Yellow,QString("Can not calculate time until storm in range")));
	       beyondRadar = false;
		    }
	     float distanceMoved = elapsedSeconds*stormSpeed/1000.0;
	     float changeInX = distanceMoved*cos(stormDirection);
	     float changeInY = distanceMoved*sin(stormDirection);
	     QString message("changeInX = "+QString().setNum(changeInX)+" changeInY = "+QString().setNum(changeInY));
	     //emit(log(Message(message,0,this->objectName())));
	     float *newLatLon = GriddedData::getAdjustedLatLon(vortexLat,vortexLon, changeInX, changeInY);
	     
	     
	     vortexLat = newLatLon[0];
	     vortexLon = newLatLon[1];
	     //Message::toScreen("New vortexLat = "+QString().setNum(vortexLat)+" New vortexLon = "+QString().setNum(vortexLon));
	   }
	 }
	 
	 QString currentCenter("Using ("+QString().setNum(vortexLat)+", "+QString().setNum(vortexLon)+") for storm center estimate");

	 emit log(Message(currentCenter,2,this->objectName()));
	 
	 // Check to see if the center is beyond 174 km
	 // If so, tell the user to wait!
	 //Message::toScreen("RLat = "+QString().setNum(*radarVolume->getRadarLat())+" RLon = "+QString().setNum(*radarVolume->getRadarLon())+" VLat = "+QString().setNum(vortexLat)+" VLon = "+QString().setNum(vortexLon));
	 float relDist = GriddedData::getCartesianDistance(
				             radarVolume->getRadarLat(), 
					     radarVolume->getRadarLon(),
					     &vortexLat, &vortexLon);
		
	 //Message::toScreen("Distance Between Radar and Storm "+QString().setNum(relDist));
	 beyondRadar = true;
	 //bool closeToEdge = false;
	 for(int i = 0; i < radarVolume->getNumSweeps(); i++) {
	   if((relDist < radarVolume->getSweep(i)->getUnambig_range())
	      &&(radarVolume->getSweep(i)->getVel_numgates()> 0)){
	     
	     beyondRadar = false;
	   }
	   //  if((relDist > radarVolume->getSweep(i)->getUnambig_range()-20)&&(relDist < radarVolume->getSweep(i)->getUnambig_range()+20)) {
	   //closeToEdge = true;
	   //}
	 }
	 
	 // Used for analytic runs only
	 if(analyticRun)
	   beyondRadar = false;
	 
	 if (beyondRadar) {
	   // Too far away for Doppler, need to send a signal
	   // Calculate the estimated time of arrival
	   float* distance;
	   distance = GriddedData::getCartesianPoint(
					    &vortexLat,&vortexLon,
					    radarVolume->getRadarLat(), 
					    radarVolume->getRadarLon());
	   float cca = atan2(distance[0], distance[1]);
	   
	   QDomElement vortex = configData->getConfig("vortex");
	   float stormSpeed = configData->getParam(vortex, 
						   "speed").toFloat()/1000.0;
	   float stormDirection = configData->getParam(vortex, 
				       "direction").toFloat()*acos(-1)/180.;
	   // Message::toScreen("Storm Direction .."+QString().setNum(stormDirection));
	   //Message::toScreen("cca = "+QString().setNum(cca));
	   float palpha = (relDist*sin(stormDirection-cca)/174.);
	   //Message::toScreen(" palpha = "+QString().setNum(palpha));
	   //Message::toScreen(" relDist = "+QString().setNum(relDist));
	   float alpha = acos(-1)-asin(palpha);
	   //Message::toScreen(" alpha = "+QString().setNum(alpha));
	   float dist2go = 0;
	   if(fabs(stormDirection-cca)<=1) {
	     dist2go = relDist-174;
	   }
	   else {
	     dist2go = 174*sin(acos(-1)+cca-stormDirection-alpha)/sin(stormDirection-cca);
	   }
	   
	   // Report distance from Doppler Radar Range

	   QString distanceLeft("Circulation Center is "+QString().setNum(dist2go)+" km from the edge of Doppler range - Skipping Analysis On This Volume");
	   emit log(Message(distanceLeft,0,this->objectName()));
	   
	   //Message::toScreen(" dist2go = "+QString().setNum(dist2go));
	   
	   if((stormSpeed ==0)||isnan(stormSpeed)||isinf(stormSpeed)) {
	     // Can not calculate time to radar edge estimate
	     // Send Message....
	     QString noTime("Unable to calculate circulation center arrival time, check circulation speed and direction");
	     emit log(Message(noTime, 0,this->objectName(),AllOff,QString(),
			      OutOfRange,QString("Storm Out Of Range")));
	   }
	   else { 
	     
	     float eta = (dist2go/stormSpeed)/60;
	     //Message::toScreen("minutes till radar"+QString().setNum(eta));
	     
	     // These is something wrong with our calculation of time
	     // until storm is in range, continue processing with
	     // warnings.
	     if((eta < 0)||(isnan(eta))||isinf(eta)) {
	       emit log(Message(QString("Difficulties Processing Time Until Radar is in Range, Please check observation parameters"),0,this->objectName(),Yellow, QString("Check Observation Parameter is Vortex Panel")));
	       if(eta < 0)
		 Message::toScreen("ETA < 0");
	       if(isnan(eta))
		 Message::toScreen("ETA is NAN");
	       if(isinf(eta))
		   Message::toScreen("ETA is infinite");
	     }
	     
	     emit log(Message(
		      QString(),
		      -1,this->objectName(),AllOff,QString(),OutOfRange, 
		      QString("Storm in range in "+QString().setNum(eta, 
						  'f', 0)+" min")));
	     //Message::toScreen("Estimated center is out of Doppler range!");
	     // We are now deleting the volume here
	     // but keeping the pointer live for future iterations
	     // have to delete here, erasing address from pollThread
	   }
	   RadarData *temp = radarVolume;
	   radarVolume = NULL;
	   delete temp;
	   
	   //Message::toScreen("AnalysisThread: Volume Not In Range - Sending Done Processing Signal");
	   
	   emit doneProcessing();
	   //Message::toScreen("AnalysisThread sent doneProcessing");
	   waitForData.wait(&mutex);
	   //Message::toScreen("AnalysisThread: Got Mutex Back For Data - Continuing");
	   mutex.unlock();
	   delete distance;
	   continue;
	 }

	 // Volume is valid and in range
	 // Prepare to process volume....
	 
	 //Message::toScreen("gets to create vortexData analysisThread");
	 // Create data instance to hold the analysis results
	 VortexData *vortexData = new VortexData(); 
	 vortexData->setTime(radarVolume->getDateTime());
	 for(int i = 0; i < vortexData->getNumLevels(); i++) {
	   vortexData->setLat(i,vortexLat);
	   vortexData->setLon(i,vortexLon);
	 }		
	 
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
		  emit log(Message(QString(),1,this->objectName()));  //6%
		  dealiaser->getConfig(configData->getConfig("qc"));
		  
		  if(dealiaser->dealias()) {
		    emit log(Message("Finished QC and Dealiasing", 2, this->objectName()));
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
			 QString name("fchar1824.dat");
			 Message::toScreen("Writing Vortrac Input "+name);
			 radarVolume->writeToFile(name);
			 Message::toScreen("Wrote Vortrac Input "+name);
         
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
			 // only good for Charley 140007
			 
			 float rt1 = 88.8294;
			 float cca1 = 63.466;
			 float rmw1 = 15.667;
			 
			 
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
			 
			 Message::toScreen("Vortex (Lat,Lon): ("+QString().setNum(vortexLat)+", "+QString().setNum(vortexLon)+")");
			 Message::toScreen("Hvvp Parameters: Distance to Radar "+QString().setNum(rt1)+" angel to vortex center in degrees ccw from north "+QString().setNum(cca1)+" rmw "+QString().setNum(rmw1));
			 
			 Hvvp *envWindFinder1 = new Hvvp;
			 envWindFinder1->setRadarData(radarVolume,rt1, cca1, rmw1);
			 if(!envWindFinder1->findHVVPWinds(true)){
			   
			 }
			 float missingLink1 = envWindFinder1->getAvAcrossBeamWinds();
			 Message::toScreen("Hvvp gives "+QString().setNum(missingLink1));
			 delete envWindFinder1;
			 
			 return;
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
		emit log(Message(QString("Creating CAPPI..."), 0, 
				 this->objectName(), Green));
		
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
		  //Message::toScreen("Before Making Cappi vLat = "+QString().setNum(vortexLat)+" vLon = "+QString().setNum(vortexLon));

		  gridData = gridFactory.makeCappi(radarVolume, configData,
						   &vortexLat, &vortexLon);
		  //Message::toScreen("AnalysisThread: outside makeCappi");
		  
		}
		
		if(abort)
		  return;
		
		// Pass Cappi to display
		emit newCappi(gridData);
		
	
		if(abort)
		  return;
		mutex.lock();

		/* GriddedData not currently a QObject so this will fail
		   connect(gridData, SIGNAL(log(const Message&)),this,
		   SLOT(catchLog(const Message&)), Qt::DirectConnection); */
		
		// Output Radar data to check if dealias worked

		gridData->writeAsi();
		emit log(Message("Done with Cappi",30,this->objectName()));
		QString cappiTime;
		cappiTime.setNum((float)analysisTime.elapsed() / 60000);
		cappiTime.append(" minutes elapsed");
		emit log(Message(cappiTime));
			 
		
		// Set the initial guess in the data object as a temporary center
		vortexData->setLat(0,vortexLat);
		vortexData->setLon(0,vortexLon);
		
		
	      
		//Find Center 
		simplexThread->findCenter(configData, gridData, radarVolume, 
					  simplexList, vortexData);
		waitForCenter.wait(&mutex); 
					 
		QString simplexTime;
		simplexTime.setNum((float)analysisTime.elapsed() / 60000);
		simplexTime.append(" minutes elapsed");
		emit log(Message(simplexTime));

		mutex.unlock();  	
		
		if (!abort) {
		  
		  mutex.lock();
		  // Get the GBVTD winds
		  emit log(Message(QString(), 1,this->objectName()));
		  vortexThread->getWinds(configData, gridData, radarVolume, 
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

		QString vortexTime;
		vortexTime.setNum((float)analysisTime.elapsed() / 60000);
		vortexTime.append(" minutes elapsed");
		emit log(Message(vortexTime));

				// Easiest Thresholding

		bool hasConvergingCenters = false;
		for(int l = 0; (l < vortexData->getNumLevels())
		      &&(hasConvergingCenters==false); l++) {
		  if(vortexData->getNumConvergingCenters(l)>0)
		    hasConvergingCenters = true;
		}
		
		if(hasConvergingCenters) {
		  vortexList->append(*vortexData);	
		  vortexList->save();
		}
		else {
		  emit log(Message(QString("Insufficient Convergence of Simplex Centers"),0,this->objectName(),AllOff,QString(),SimplexError,QString("Could Not Obtain Center Convergence")));
		}
		  
		if(!hasConvergingCenters && (vortexList->count() > 0)){
		  //vortexList->removeAt(vortexList->count()-1);
		  //vortexList->save();		
		  //simplexList->removeAt(simplexList->count()-1);
		  //simplexList->save();
		  emit log(Message(QString("Simplex Analysis Found No Converging Centers in this volume"),0,this->objectName(),Green, QString("No Center Found!"),OutOfRange, QString("No Converging Centers")));
		  analysisGood = false;
		}
		
		//Message::toScreen("The number of vortexData in the List is "+QString().setNum(vortexList->count()));

		  // Moved this to above last if

		//vortexList->append(*vortexData);	
		//vortexList->save();
		
		// Delete CAPPI and RadarData objects
		
		RadarData *temp = radarVolume;
		radarVolume = NULL;
		delete temp;
		
		delete gridData;
		delete vortexData;
		
		//Message::toScreen("Deleted vortex data.... ???");
		
		if(!analysisGood)
		{
			// Some error occurred, notify the user
			emit log(Message("Radar volume processing error!"));
			emit log(Message(
			   QString("Radar volume processing error!!"), 0,
			   this->objectName()));
			//Message::toScreen("AnalysisThread: Analysis BAD, Done Processing");
			emit(doneProcessing());
		       
			//return; // I got rid of this
			// do we need to fail if the 
			// volume is bad? -LM 1/20/07
		} else {
			// Store the resulting analysis in the vortex list
			archiveAnalysis();
			
			// Complete the progress bar and log that we're done
			emit log(Message(QString("Analysis complete!"),8,
					 this->objectName(), AllOff, 
					 QString(),Ok, QString()));

			// Let the poller know we're done
			//Message::toScreen("AnalysisThread: Analysis GOOD, Done Processing");
			emit(doneProcessing());
		}
		mutex.unlock();
		
		// Go to sleep, wait for more data
		if (!abort) {
		  mutex.lock();
		  // Wait until new data is available
		  waitForData.wait(&mutex);
		  //Message::toScreen("AnalysisThread: Received the Signal To Move Ahead with New Data");
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
  numVolProcessed = (int)num;
}

void AnalysisThread::setAnalyticRun(const bool& runOnce)
{
  analyticRun = runOnce;
}
