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
  this->setObjectName("Analysis");
  //Message::toScreen("AnalysisThread Constructor");
  abort = false;
  /* simplexThread = new SimplexThread;
  connect(simplexThread, SIGNAL(log(const Message&)), this, 
		  SLOT(catchLog(const Message&)));
  connect(simplexThread, SIGNAL(centerFound()), 
		  this, SLOT(foundCenter()),Qt::DirectConnection);
  vortexThread = new VortexThread;
  connect(vortexThread, SIGNAL(log(const Message&)), this, 
		  SLOT(catchLog(const Message&)));
  connect(vortexThread, SIGNAL(windsFound()), 
		  this, SLOT(foundWinds()),Qt::DirectConnection); */

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

/*  delete simplexThread; // This goes first so it can break the cycle which will
						// allow us a change to lock if we are in simplexThread
						// Otherwise we could wait a while for a lock
  simplexThread = NULL;
  
  delete vortexThread;    
  vortexThread = NULL; */ 

  //delete radarVolume;
  //radarVolume = NULL;
  
  this->quit();
  
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

void AnalysisThread::setSimplexThread(SimplexThread *threadPtr)
{
	
	simplexThread = threadPtr;

}

void AnalysisThread::setVortexThread(VortexThread *threadPtr)
{
	
	vortexThread = threadPtr;
	
}

void AnalysisThread::abortThread()
{
  //Message::toScreen("In AnalysisThread Abort");
  
  mutex.lock();
  abort = true;
  mutex.unlock();
  
  // Wait for the thread to finish running if it is still processing
  if(this->isRunning()) {
    //Message::toScreen("This is running - analysisThread");
    waitForData.wakeOne();
    // Message::toScreen("WaitForData.wakeAll() - passed");
    //wait();
    //Message::toScreen("Got past wait - AnalysisThread");
    // Got rid of wait because it was getting stuck when aborted in simplex cycle
	/* Done in Destructor now
    if(radarVolume!=NULL) {
      RadarData *temp = radarVolume;
      radarVolume = NULL;
      delete temp;
    } */
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
	 emit log(Message(QString(),0,this->objectName(),Green));
	 QTime analysisTime;
	 analysisTime.start();    // Timer used for user info only
	 
	 bool beyondRadar;
	 
	 // Check the vortexList for a current center to use
	 // as the starting point for analysis

	 if (!vortexList->isEmpty()) {
	   vortexList->timeSort();
	   vortexLat = vortexList->last().getLat();
	   vortexLon = vortexList->last().getLon();
	   if(numVolProcessed > 0) {
	     // If only a few volumes have been attempted then we will use 
	     // the storm motion to 'bump' the storm into range
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
	     
	     QDateTime obsDateTime = vortexList->last().getTime();
	     QDateTime volDateTime = radarVolume->getDateTime();

	     int elapsedSeconds =obsDateTime.secsTo(volDateTime);
	     //Message::toScreen("Seconds since start "+QString().setNum(elapsedSeconds)+" in AnalysisThread");
	     if(isnan(elapsedSeconds)) {
	       emit log(Message(QString("Cannot calculate time until storm is in range of radar, Please check the observation time, latitude, longitude, and storm movement parameters"),0,this->objectName(),Yellow,QString("Can not calculate time until storm in range")));
	       beyondRadar = false;
		    }
	     float distanceMoved = elapsedSeconds*stormSpeed/1000.0;
	     float changeInX = distanceMoved*cos(stormDirection);
	     float changeInY = distanceMoved*sin(stormDirection);
	     QString message("changeInX = "+QString().setNum(changeInX)+" changeInY = "+QString().setNum(changeInY));
	     //emit(log(Message(message,0,this->objectName())));
	     float *newLatLon = GriddedData::getAdjustedLatLon(vortexLat,vortexLon, changeInX, changeInY);

		 // Get initial lat and lon
	     float initLat = configData->getParam(vortex,"lat").toFloat();
	     float initLon = configData->getParam(vortex,"lon").toFloat();
	     QString dateString = configData->getParam(vortex,"obsdate");
	     QString timeString = configData->getParam(vortex,"obstime");
	     QDate obsDate = QDate::fromString(configData->getParam(vortex,"obsdate"),"yyyy-MM-dd");
	     //Message::toScreen("obs: "+obsDate.toString("yyyy-MM-dd"));
	     QTime obsTime = QTime::fromString(timeString,"hh:mm:ss");
	     //Message::toScreen("obs: "+obsTime.toString("hh:mm:ss"));
	     obsDateTime = QDateTime(obsDate, obsTime, Qt::UTC);
	     if(!obsDateTime.isValid()){
	       emit log(Message(QString("Observation Date or Time is not of valid format! Date: yyyy-MM-dd Time: hh:mm:ss please adjust the configuration file"),0,this->objectName(),Red, QString("ObsDate or ObsTime invalid in Config")));
	       mutex.unlock();
	       abort = true;
	       return;
	     }
	     elapsedSeconds =obsDateTime.secsTo(volDateTime);
	     //Message::toScreen("Seconds since start "+QString().setNum(elapsedSeconds)+" in AnalysisThread");
	     if(isnan(elapsedSeconds)) {
	       emit log(Message(QString("Cannot calculate time until storm is in range of radar, Please check the observation time, latitude, longitude, and storm movement parameters"),0,this->objectName(),Yellow,QString("Can not calculate time until storm in range")));
	       beyondRadar = false;
		    }
	     distanceMoved = elapsedSeconds*stormSpeed/1000.0;
	     changeInX = distanceMoved*cos(stormDirection);
	     changeInY = distanceMoved*sin(stormDirection);
	     float *extrapLatLon = GriddedData::getAdjustedLatLon(initLat,initLon, changeInX, changeInY);
	     float relDist = GriddedData::getCartesianDistance(&extrapLatLon[0],&extrapLatLon[1],&newLatLon[0],&newLatLon[1]);

		 if (relDist > 10) {
			 QString distString;
			 QString M1 = "Previous Simplex center "+distString.setNum((int)relDist)+" (>10 km) from User estimated center, may be lost or need to update estimate";
			 QString M2 = "Last Simplex & Estimate "+distString.setNum((int)relDist)+" km apart";
			 emit log(Message(M1,0,this->objectName(),Yellow, M2));
			 vortexLat = extrapLatLon[0];
			 vortexLon = extrapLatLon[1];
		 } else {
	     // Message::toScreen("Old vortexLat = "+QString().setNum(vortexLat)+" Old vortexLon = "+QString().setNum(vortexLon));
	     // Message::toScreen("New vortexLat = "+QString().setNum(newLatLon[0])+" New vortexLon = "+QString().setNum(newLatLon[1]));
			 vortexLat = newLatLon[0];
			 vortexLon = newLatLon[1];
		 }
	     delete [] newLatLon;
	     delete [] extrapLatLon;
	   }
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
		   mutex.unlock();
		   abort = true;
		   return;
	   }

	   QString startDate = configData->getParam(radar,"startdate");
	   QString startTime = configData->getParam(radar,"starttime");
	   QDateTime radarStart(QDate::fromString(startDate, Qt::ISODate), 
				QTime::fromString(startTime, Qt::ISODate), 
				Qt::UTC);

	   // Get this volume's time
	   //Message::toScreen("obs: "+obsDateTime.toString(Qt::ISODate));
	   
	   QDateTime volDateTime = radarVolume->getDateTime();
	   //Message::toScreen("vol: "+volDateTime.toString(Qt::ISODate));
	   
	   if(analyticRun) {
		   // Message::toScreen("Using lat lon in Config");
		   vortexLat = configData->getParam(vortex,"lat").toFloat();
		   vortexLon = configData->getParam(vortex,"lon").toFloat();
	   } else {
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
			   emit log(Message(QString("Error extrapolating user center, Please check the observation time, latitude, longitude, and storm movement parameters"),0,this->objectName(),Red,QString("Error extrapolating center")));
			   beyondRadar = false;
		   } else if (elapsedSeconds > (3600*6)) {
			   // Trying to extrapolate too far, warn
			   emit log(Message(QString("Extrapolation time exceeds six hours, Please check the observation time, latitude, longitude, and storm movement parameters"),0,this->objectName(),Yellow,QString("Extrapolation time > 6 hrs")));
		   } else if (elapsedSeconds < 0) {
			   // Problem, most likely being run in post-analysis mode
			   emit log(Message(QString("Negative time extrapolation, Please check the observation time, latitude, longitude, and storm movement parameters"),0,this->objectName(),Yellow,QString("Negative extrapolation of center")));
		   }
		   float distanceMoved = elapsedSeconds*stormSpeed/1000.0;
		   float changeInX = distanceMoved*cos(stormDirection);
		   float changeInY = distanceMoved*sin(stormDirection);
		   QString message("changeInX = "+QString().setNum(changeInX)+" changeInY = "+QString().setNum(changeInY));
		   //emit(log(Message(message,0,this->objectName())));
		   float *newLatLon = GriddedData::getAdjustedLatLon(vortexLat,vortexLon, changeInX, changeInY);
		   
		   vortexLat = newLatLon[0];
		   vortexLon = newLatLon[1];
		   delete [] newLatLon;
		   //Message::toScreen("New vortexLat = "+QString().setNum(vortexLat)+" New vortexLon = "+QString().setNum(vortexLon));
	   }
	 }
		   
	 QString currentCenter("Using ("+QString().setNum(vortexLat)+", "+QString().setNum(vortexLon)+") for storm center estimate");

	 emit log(Message(currentCenter,1,this->objectName())); // 5 %
	 
	 // Prepare to process volume....
	 
	 //Message::toScreen("gets to create vortexData analysisThread");
	 // Create data instance to hold the analysis results
	 
	 // Use size hints from vtd panel
	 QDomElement vtd = configData->getConfig("vtd");
	 QDomElement cappi = configData->getConfig("cappi");
	 float top = configData->getParam(vtd, "toplevel").toFloat();
	 float bottom = configData->getParam(vtd, "bottomlevel").toFloat();
	 float zSpacing = configData->getParam(cappi, "zgridsp").toFloat();
	 int numLevels = (int)floor((top-bottom)/zSpacing + 1.5);
	 float inner = configData->getParam(vtd, "innerradius").toFloat();
	 float outer = configData->getParam(vtd, "outerradius").toFloat();
	 float ringwidth = configData->getParam(vtd, "ringwidth").toFloat();
	 int numRings = (int)floor((outer-inner)/ringwidth + 1.5);
	 int numWaveNum = configData->getParam(vtd,"maxwavenumber").toInt();
	 VortexData *vortexData = new VortexData(numLevels,numRings,
						 numWaveNum);
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
			  SLOT(catchLog(const Message&)));
		  dealiaser->getConfig(configData->getConfig("qc"));
		  
		  if(dealiaser->dealias()) {
		    emit log(Message("Finished QC and Dealiasing", 
				     1, this->objectName()));  // 10 %
		    radarVolume->isDealiased(true);
		  } else {
		    emit log(Message(QString("Finished Dealias Method with Failures"),0,this->objectName()));
		    analysisGood = false;
		    // Something went wrong
		    // We should probably add a return here of some sort...
		  }
		  delete dealiaser;
		}
		else
		  emit log(Message(QString("RadarVolume is Dealiased"),0,this->objectName()));
				
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
				 this->objectName()));
		
		/*  If Analytic Model is running we need to make an analytic
		 *  gridded data rather than a cappi
		 */
		//GriddedData *gridData;
			
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
		   
		emit log(Message("Done with Cappi",15,this->objectName()));

		// Pass Cappi to display
		emit newCappi(gridData);
		//emit newCappiInfo(.3,.3,.1,.2,.3);
		
	
		if(abort)
		  return;
		mutex.lock();
		
		// Output Radar data to check if dealias worked

		gridData->writeAsi();
		emit log(Message("Wrote Cappi To File",5,this->objectName()));
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

		bool hasConvergingCenters = false;
		for(int l = 0; (l < vortexData->getNumLevels())
			&&(hasConvergingCenters==false); l++) {
			if(vortexData->getNumConvergingCenters(l)>0)
				hasConvergingCenters = true;
		}
		   
		QString simplexTime;
		simplexTime.setNum((float)analysisTime.elapsed() / 60000);
		simplexTime.append(" minutes elapsed");
		emit log(Message(simplexTime));
		int vortexIndex = vortexData->getHeightIndex(1);
		if(vortexIndex == -1)
			vortexIndex = 0;
		float levelLat = vortexData->getLat(vortexIndex);
		float levelLon = vortexData->getLon(vortexIndex);
		   
		float relDist = GriddedData::getCartesianDistance(&vortexLat,&vortexLon,&levelLat,&levelLon);
		if (relDist > 999) relDist = 999; 
		if (relDist > 50) {
			QString distString, latString, lonString;
			QString M1 = "Simplex center ("+latString.setNum(levelLat)+", "+lonString.setNum(levelLon)+ ")"
				" > 50 km ("+distString.setNum((int)relDist)+" km) from user estimate, assuming Simplex is lost and attempting user center";
			QString M2 = "Simplex center "+distString.setNum((int)relDist)+" km away rejected";
			emit log(Message(M1,0,this->objectName(),Yellow, M2));
			vortexData->setLat(vortexIndex, vortexLat);
			vortexData->setLon(vortexIndex, vortexLon);
			levelLat = vortexLat;
			levelLon = vortexLon;
			hasConvergingCenters = false;
		} else if (relDist > 10) {
			QString distString;
			QString M1 = "Simplex center "+distString.setNum((int)relDist)+" (> 10 km) from User estimated center, may be lost or need to update estimate";
			QString M2 = "Center/Estimate "+distString.setNum((int)relDist)+" km apart";
			emit log(Message(M1,0,this->objectName(),Yellow, M2));
		}
		   
		// Send info about simplex search to cappiDisplay 
		QDomElement radar = configData->getConfig("radar");
		QDomElement simplex = configData->getConfig("center");
		float radarLat = configData->getParam(radar,"lat").toFloat();
		float radarLon = configData->getParam(radar,"lon").toFloat();
		float* xyValues = gridData->getCartesianPoint(&radarLat, &radarLon, &levelLat, &levelLon);
		float xPercent = float(gridData->getIndexFromCartesianPointI(xyValues[0])+1)/gridData->getIdim();
		//Message::toScreen("xPercent is "+QString().setNum(xPercent));
		float yPercent = float(gridData->getIndexFromCartesianPointJ(xyValues[1])+1)/gridData->getJdim();
		//Message::toScreen("yPercent is "+QString().setNum(yPercent));
		float rmwEstimate = vortexData->getRMW(vortexIndex)/(gridData->getIGridsp()*gridData->getIdim());
		//Message::toScreen("sMin is "+QString().setNum(sMin));
		float sMin = configData->getParam(simplex, "innerradius").toFloat()/(gridData->getIGridsp()*gridData->getIdim());
		float sMax = configData->getParam(simplex, "outerradius").toFloat()/(gridData->getIGridsp()*gridData->getIdim());
		float vMax = configData->getParam(vtd, "outerradius").toFloat()/(gridData->getIGridsp()*gridData->getIdim());
		//Message::toScreen("vMax is "+QString().setNum(vMax));
		emit newCappiInfo(xPercent, yPercent, rmwEstimate, sMin, sMax, vMax, vortexLat, vortexLon, levelLat, levelLon);
		delete [] xyValues;
		
		
		if(!hasConvergingCenters) { // && (vortexList->count() > 0)){
			//emit log(Message(QString("Simplex Analysis Found No Converging Centers in this volume"),0,this->objectName(),Yellow, QString("No Center Found!"),OutOfRange, QString("No Converging Centers")));
			emit log(Message(QString("Insufficient Convergence of Simplex Centers"),0,this->objectName(),AllOff,QString(),SimplexError,QString("No Center Convergence")));
			//analysisGood = false;
			// Delete CAPPI and RadarData objects			
			delete radarVolume;
			delete gridData;
			delete vortexData;			
			emit(doneProcessing());
		} else {		   
			
			mutex.unlock();  	
			
			if (!abort) {
				
				mutex.lock();
				// Get the GBVTD winds
				emit log(Message(QString(), 2,this->objectName()));
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
			
			emit log(Message(QString(),2,this->objectName())); // 97 %
			
			
			vortexList->append(*vortexData);	
			vortexList->save();
			
			// Print out summary information to log
			QString summary = "VORTRAC ATCF,";
			QString values;
			summary += vortexData->getTime().toString(Qt::ISODate) + ",";
			summary += values.setNum(vortexData->getLat()) + ",";
			summary += values.setNum(vortexData->getLon()) + ",";
			summary += values.setNum(vortexData->getPressure()) + ",";
			summary += values.setNum(vortexData->getPressureUncertainty()) + ",";
			summary += values.setNum(vortexData->getRMW()) + ",";
			summary += values.setNum(vortexData->getRMWUncertainty());
			emit log(Message(summary,0,this->objectName()));

			// Delete CAPPI and RadarData objects			
			delete radarVolume;
			delete gridData;
			delete vortexData;
			
			//Message::toScreen("Deleted vortex data.... ???");
			
			if(!analysisGood)
			{
				// Some error occurred, notify the user
				emit log(Message("Radar volume processing error!"));
				emit log(Message(
								 QString("Radar volume processing error!!"), 2,
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
				emit log(Message(QString("Analysis complete!"),2,
								 this->objectName(), AllOff, 
								 QString(),Ok, QString()));
				
				// Let the poller know we're done
				//Message::toScreen("AnalysisThread: Analysis GOOD, Done Processing");
				emit(doneProcessing());
			}
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
