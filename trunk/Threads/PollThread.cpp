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
#include <math.h>

#include "DataObjects/SimplexList.h"

PollThread::PollThread(QObject *parent)
  : QThread(parent)
{
  this->setObjectName("Poller");
  //  emit log(Message(QString("Constructor"),0,this->objectName()));
  abort = false;
  runOnce = false;
  processPressureData = true;
   analysisThread = NULL;

  dataSource= NULL;
  pressureSource= NULL;
  configData= NULL;
  vortexList= NULL;
  simplexList= NULL;
  pressureList= NULL;
  dropSondeList= NULL;
  vortexConfig= NULL;
  simplexConfig= NULL;
  pressureConfig= NULL;
  dropSondeConfig= NULL;
}

PollThread::~PollThread()
{
  //emit log(Message(QString("INTO Destructor"),0,this->objectName()));
  mutex.lock();
  abort = true;
  processPressureData = false;
  mutex.unlock();
  //  if(this->isRunning())
  this->abortThread();
  /* Deleting Members
	  - Causes crash when explicitly deleting -MB */
    
  delete simplexThread; // This goes first so it can break the cycle which will
	 // allow us a change to lock if we are in simplexThread
	 // Otherwise we could wait a while for a lock
  simplexThread = NULL;
	 
  delete vortexThread;    
  vortexThread = NULL;

  delete analysisThread;
  analysisThread = NULL;

  delete vortexList;
  vortexList = NULL;
  
  delete simplexList;
  simplexList = NULL;
  
  delete pressureList;
  pressureList = NULL;
  
  delete dropSondeList;
  dropSondeList = NULL;
  
  
  this->quit();
	
  /*else
    {
    wait();
    
    // Deleting Members
    delete analysisThread;
    configData = NULL;
    delete configData;
    delete vortexList;
    delete simplexList;
    delete pressureList;
    delete dropSondeList
    delete vortexConfig;  // This should be deleted do we need to save?
    delete simplexConfig;  // This should be deleted do we need to save?
    delete pressureConfig;   // This should be deleted do we need to save?
    delete dropSondeConfig;   // This should be deleted do we need to save?
    }
  */
  //emit log(Message(QString("OUT OF Destructor"),0,this->objectName()));
}

void PollThread::setConfig(Configuration *configPtr)
{

  configData = configPtr;

}

void PollThread::abortThread()
{
  //emit log(Message(QString("Enter ABORT"),0,this->objectName()));
  mutex.lock();
  abort = true;
  processPressureData = false;
  mutex.unlock();
 
  //this->exit();
  if(this->isRunning()) {
    //Message::toScreen("PollThread: Wait For Analysis Wake All");
    waitForAnalysis.wakeAll();  // What does this do ? -LM
  }
  wait();

  
  //emit log(Message(QString("Leaving Abort"),0,this->objectName()));
 
}

void PollThread::analysisDoneProcessing()
{
  //Message::toScreen("PollThread: Wait For Analysis Wake One");
  processPressureData = false;
  waitForAnalysis.wakeOne();
}

void PollThread::run()
{

  abort = false;
  emit log(Message(QString("Polling for data..."), 0, this->objectName()));
  dataSource = new RadarFactory(configData);
  connect(dataSource, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)));

  PressureFactory *pressureSource = new PressureFactory(configData);
  connect(pressureSource, SIGNAL(log(const Message&)),
		  this, SLOT(catchLog(const Message&)));

  if(!continuePreviousRun) {
    QString file("vortrac_defaultVortexListStorage.xml");
    vortexConfig = new Configuration(0, file);
    vortexConfig->setObjectName("Vortex Configuration");
    vortexConfig->setLogChanges(false);
    connect(vortexConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)));
    vortexList = new VortexList(vortexConfig);
    vortexList->open();
  
    file = QString("vortrac_defaultSimplexListStorage.xml");
    //simplexConfig = new Configuration(0,QString());
    simplexConfig = new Configuration(0, file);
    simplexConfig->setObjectName("Simplex Configuration");
    simplexConfig->setLogChanges(false);
    connect(simplexConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)));
    simplexList = new SimplexList(simplexConfig);
    simplexList->open();
    
    file = QString("vortrac_defaultPressureListStorage.xml");
    //pressureConfig = new Configuration(0,QString());
    pressureConfig = new Configuration(0, file);
    pressureConfig->setObjectName("Pressure Configuration");
    pressureConfig->setLogChanges(false);
    connect(pressureConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)));
    pressureList = new PressureList(pressureConfig);
    pressureList->open();

    dropSondeConfig = new Configuration(0, file);
    dropSondeConfig->setObjectName("Dropsonde Configuration");
    dropSondeConfig->setLogChanges(false);
    connect(dropSondeConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)));
    dropSondeList = new PressureList(dropSondeConfig);
    dropSondeList->open();
  
  }
  else {

    // Gather information about the files in the working directory so we can 
    // load information about a previous run

    QString workingDirectoryPath = configData->getParam(configData->getConfig("vortex"),"dir");
    QDir workingDirectory(workingDirectoryPath);
    QString vortexName = configData->getParam(configData->getConfig("vortex"), "name");
    QString radarName = configData->getParam(configData->getConfig("radar"), "name");
    QString year;
    year.setNum(QDate::fromString(configData->getParam(configData->getConfig("radar"),"startdate"), "yyyy-MM-dd").year());

    QString nameFilter = vortexName+"_"+radarName+"_"+year;
    QStringList allPossibleFiles = workingDirectory.entryList(QDir::Files);
    allPossibleFiles = allPossibleFiles.filter(nameFilter, Qt::CaseInsensitive);

    QString file = allPossibleFiles.filter("vortexList").value(0);
    emit log(Message(QString("Loading Vortex List from File: "+file),0,this->objectName()));
    vortexConfig = new Configuration(0, workingDirectory.filePath(file));
    vortexConfig->setObjectName("vortexConfig");
    vortexConfig->setLogChanges(false);
    connect(vortexConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)));
    vortexList = new VortexList(vortexConfig);
    vortexList->open();
    
    QString vortexPath = configData->getParam(configData->getConfig("vtd"), 
					      "dir");
    QDir vortexWorkingDirectory(vortexPath);
    QString outFileName = workingDirectory.path()+"/";
    outFileName += vortexName+"_"+radarName+"_"+year+"_vortexList.xml";
    vortexList->setFileName(outFileName);
    vortexList->setNewWorkingDirectory(vortexWorkingDirectory.path()+"/");
  
    file = allPossibleFiles.filter("simplexList").value(0);
    emit log(Message(QString("Loading Simplex List from File: "+file),0,this->objectName()));
    simplexConfig = new Configuration(0, workingDirectory.filePath(file));
    simplexConfig->setObjectName("simplexConfig");
    simplexConfig->setLogChanges(false);
    connect(simplexConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)));
    simplexList = new SimplexList(simplexConfig);
    
    simplexList->open();

    QString simplexPath = configData->getParam(configData->getConfig("center"),
					       "dir");
    QDir simplexWorkingDirectory(simplexPath);

    outFileName = workingDirectory.path() + "/";
    outFileName += vortexName+"_"+radarName+"_"+year+"_simplexList.xml";
    simplexList->setFileName(outFileName);
    simplexList->setNewWorkingDirectory(simplexWorkingDirectory.path()+"/");
    
    if(allPossibleFiles.filter("pressureList").count() > 0) {
      file = allPossibleFiles.filter("pressureList").value(0);
      pressureConfig = new Configuration(0, workingDirectory.filePath(file));
    }
    else {
      file = QString("vortrac_defaultPressureListStorage.xml");
      pressureConfig = new Configuration(0, QDir::current().filePath(file));
    }
   
    emit log(Message(QString("Loading Pressure List from File: "+file),0,this->objectName()));
    pressureConfig->setObjectName("pressureConfig");
    pressureConfig->setLogChanges(false);
    connect(pressureConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)));
    pressureList = new PressureList(pressureConfig);
    pressureList->open();
    emit log(Message(QString("Number of pressure obs loaded: "+QString().setNum(pressureList->count()))));
    //Message::toScreen("Number of pressure obs loaded "+QString().setNum(pressureList->count()));
    
    outFileName = workingDirectory.path()+"/";
    outFileName+= vortexName+"_"+radarName+"_"+year+"_pressureList.xml";
    pressureList->setFileName(outFileName);
    pressureList->setNewWorkingDirectory(workingDirectory.path());

    if(allPossibleFiles.filter("dropSondeList").count() > 0) {
      file = allPossibleFiles.filter("dropSondeList").value(0);
      dropSondeConfig = new Configuration(0, workingDirectory.filePath(file));
    }
    else {
      file = QString("vortrac_defaultPressureListStorage.xml");
      dropSondeConfig = new Configuration(0, QDir::current().filePath(file));
    }
    //emit log(Message(file,0,this->objectName()));
    dropSondeConfig->setObjectName("dropSondeConfig");
    dropSondeConfig->setLogChanges(false);
    connect(dropSondeConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)));
    dropSondeList = new PressureList(dropSondeConfig);
    dropSondeList->open();

    outFileName = workingDirectory.path()+"/";
    outFileName += vortexName+"_"+radarName+"_"+year+"_dropSondeList.xml";
    dropSondeList->setFileName(outFileName);
    dropSondeList->setNewWorkingDirectory(workingDirectory.path());

    checkListConsistency();

    emit vortexListUpdate(vortexList);
    
    checkIntensification();

  }  

  analysisThread = new AnalysisThread;
  connect(analysisThread, SIGNAL(doneProcessing()), 
  	  this, SLOT(analysisDoneProcessing()));
  connect(analysisThread, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)));
  connect(this, SIGNAL(terminated()),
	  analysisThread, SLOT(terminate()));
  connect(analysisThread, SIGNAL(newVCP(const int)),
	  this, SLOT(catchVCP(const int)));
  connect(analysisThread, SIGNAL(newCappi(const GriddedData*)),
	  this, SLOT(catchCappi(const GriddedData*)));
  connect(analysisThread, SIGNAL(newCappiInfo(const float&, const float&, 
					      const float&, const float&, const float&, const float&,  
					      const float&, const float&, const float&, const float& )),
	  this, SLOT(catchCappiInfo(const float&, const float&, const float&, const float&, 
				    const float&, const float&, const float&, const float&,
					const float&, const float& )));
  
  analysisThread->setVortexList(vortexList);
  analysisThread->setSimplexList(simplexList);
  analysisThread->setPressureList(pressureList);
  analysisThread->setDropSondeList(dropSondeList);
  analysisThread->setAnalyticRun(runOnce);

    simplexThread = new SimplexThread;
	connect(simplexThread, SIGNAL(log(const Message&)), this, 
			SLOT(catchLog(const Message&)));
	connect(simplexThread, SIGNAL(centerFound()), 
			analysisThread, SLOT(foundCenter()),Qt::DirectConnection);
	vortexThread = new VortexThread;
	connect(vortexThread, SIGNAL(log(const Message&)), this, 
			SLOT(catchLog(const Message&)));
	connect(vortexThread, SIGNAL(windsFound()), 
			analysisThread, SLOT(foundWinds()),Qt::DirectConnection);
	analysisThread->setSimplexThread(simplexThread);
	analysisThread->setVortexThread(vortexThread);
	
	// Begin polling loop
	forever {
	  //Message::toScreen("PollThread: Begining Again In Forever");
	  
     	  // Check for new data
	  if (dataSource->hasUnprocessedData()) {
	    emit log(Message("Data found, starting analysis...", -1, 
			     this->objectName()));	       

	    mutex.lock();
	    // Update the data queue with any knowledge of any volumes that
	    // might have already been processed

	    dataSource->updateDataQueue(vortexList);
	    analysisThread->setNumVolProcessed(dataSource->getNumProcessed());
	        
	    // Select a volume off the queue
	    RadarData *newVolume = dataSource->getUnprocessedData();
	    if(newVolume == NULL) {
	      delete newVolume;
	      mutex.unlock();
	      continue;
	    }
		  QString fileName = "Found file:" + newVolume->getFileName();  
		emit log(Message(fileName, 1, this->objectName()));

	    //emit log(Message(QString(),1,this->objectName()));
	    
	    // Check to makes sure that the file still exists and is readable
	    
	    if(!newVolume->fileIsReadable()) {
	      emit log(Message(QString("The radar data file "+newVolume->getFileName()+" is not readable"), -1,this->objectName()));
	      delete newVolume;
	      mutex.unlock();
	      continue;
	    }

	    // Read the volume from file into the RadarData format
	    newVolume->readVolume();
	    QString radarFileName(newVolume->getFileName());
	    QDateTime volTime = newVolume->getDateTime();
	    
	    // Check to make sure that radar volume is still within the
	    // start and stop dates specified in the configuration.
	    QDomElement radar = configData->getConfig("radar");
	    QString startDate = configData->getParam(radar,"startdate");
	    QString startTime = configData->getParam(radar,"starttime");
	    QString stopDate = configData->getParam(radar, "enddate");
	    QString stopTime = configData->getParam(radar, "endtime");
	    QDateTime radarStart(QDate::fromString(startDate, Qt::ISODate), 
				 QTime::fromString(startTime, Qt::ISODate), 
				 Qt::UTC);

	    QDateTime radarStop(QDate::fromString(stopDate,Qt::ISODate),
				QTime::fromString(stopTime, Qt::ISODate),
				Qt::UTC);
	    if((volTime < radarStart)||(volTime > radarStop)) {
	      delete newVolume;
	      mutex.unlock();
	      continue;
	    }

	    emit log(Message(QString(),3,this->objectName()));
	    mutex.unlock();
	    // Send the file to AnalysisThread for processing
	    analysisThread->analyze(newVolume,configData);
	    mutex.lock();

	    if (!abort) {
	      
	      // Check for new pressure data to process while we are
	      //    analyzing the current volume

	      if(processPressureData) {
		//Message::toScreen("Decided to search for pressure data");
		while (pressureSource->hasUnprocessedData()
		       && processPressureData) {
		  
		  // Create a list of new pressure observations that 
		  //    have not yet been processed
		  
		  QList<PressureData>* newObs = pressureSource->getUnprocessedData();
		  // Add any new observations to the list of observations
		  //    which are used to calculate the current pressure
		  
		  for (int i = newObs->size()-1;i>=0; i--) {
		    bool match = false;
		    for(int j = 0; (!match)&&(j < pressureList->size()); j++) {
		      if(pressureList->value(j)==newObs->value(i)) {
			match = true;
		      }
		    }
		    if(!match) {
		      pressureList->append(newObs->at(i));
		    }
		  }
		  delete newObs;
		  
		  // If processPressureData is still true
		  // wait for 30 sec and check again
		  // make sure to exit loop in the analysisThread
		  // returns with the data.

		  if(processPressureData)
		    //Message::toScreen("PollThread: Finished Set Of Pressure Data - Waiting For Mutex Or 1 Min");
		    waitForAnalysis.wait(&mutex, 30000);
		  //Message::toScreen("PollThread: Finished Waiting After Finished Set");
		    		  
		  // Otherwise bail out of the loop because we are done 
		  // done in analysisThread
		}
		if(processPressureData) {
		  // If this is still true it means that there is no data
		  // to process but AnalysisThread is still working.
		  // So we must wait
		  //Message::toScreen("PollThread: Waiting For Mutex Only");
		  waitForAnalysis.wait(&mutex);
		  //Message::toScreen("PollThread: Done Waiting For Mutex Only");
		  
		}
	      }
	      
	      else {
		//Message::toScreen("Decided NOT to search for pressure data");
		// If we don't need to process pressure data
		// then we will just wait for analysisThread
		waitForAnalysis.wait(&mutex);
	      }

	      //Message::toScreen("PollThread: Stopped Stalling In Pressure Loop");
	      
	      // Process pressure data next time
	      processPressureData = true;
	                   
	      // Saving the new data entries
	      // PressureList will not save if no volumes complete analysis
	      // Moving the pressureList save step further up in processing
	      // could result in serious synchronization issues.
	      
	      if (!pressureList->getFileName().isNull()) 	
		pressureList->save();
	      
	      // Done with radar volume, send a signal to the Graph to update
	      emit vortexListUpdate(vortexList);
	      emit log(Message(QString("Completed Analysis On Volume "+radarFileName),100,
			       this->objectName(),Green));
	    }
	    mutex.unlock();  

	  // Check to see if the new volume affects the storm trend
	  checkIntensification();
	  emit log(Message(QString(),100,this->objectName()));  // 100 % 
	  // Delete the radar volume here, not in AnalysisThread
	  delete newVolume;

	  }
	  
		
	  // Check to see if we should quit
	  if (abort) {
	    delete dataSource;
	    delete pressureSource;
	    return;
	  }
	  if (runOnce) {
	    emit log(Message(tr("Analysis Completed Exiting PollThread"),-1));
	    delete dataSource;
	    delete pressureSource;
	    runOnce = false;
	    return;
	  }
	}
	
	emit log(Message(QString("PollThread Finished"),-1,this->objectName()));	
	delete dataSource;
	delete pressureSource;
}

// This slot is used for log message relaying
// Any objects created by this object must be connected
// to this slot

void PollThread::catchLog(const Message& message)
{
  emit log(message);
}


void PollThread::catchVCP(const int vcp)
{
  emit newVCP(vcp);
}

void PollThread::catchCappi(const GriddedData* cappi)
{
	emit newCappi(cappi);
}

void PollThread::setOnlyRunOnce(const bool newRunOnce) {
  mutex.lock();
  runOnce = newRunOnce;
  mutex.unlock();
}

void PollThread::setContinuePreviousRun(const bool &decision)
{
  continuePreviousRun = decision;
}

void PollThread::checkIntensification()
{
  // Checks for any rapid changes in pressure
  
  QDomElement pressure = configData->getConfig("pressure");

  // Units of mb / hr
  float rapidRate = configData->getParam(pressure, QString("rapidlimit")).toFloat();

  if(isnan(rapidRate)) {
    emit log(Message(QString("Could Not Find Rapid Intensification Rate, Using 3 mb/hr"),
		     0,this->objectName()));
    rapidRate = 3.0;
  }
  
  // So we don't report falsely there must be a rapid increase trend which 
  // spans several measurements

  // Number of volumes which are averaged.
  int volSpan = configData->getParam(pressure, QString("av_interval")).toInt();
  if(isnan(volSpan)) {
    emit log(Message(QString("Could Not Find Pressure Averaging Interval for Rapid Intensification, Using 8 volumes"),0,this->objectName()));
    volSpan = 8;
  }

  int lastVol = vortexList->count()-1;

  if(lastVol > 2*volSpan) {
    if(vortexList->at(int(volSpan/2.)).getTime().secsTo(vortexList->at(lastVol-int(volSpan/2.)).getTime()) > 3600) {
      float recentAv = 0;
      float pastAv = 0;
      int recentCount = 0;
      int pastCenter = 0;
      int pastCount = 0;
      for(int k = lastVol; k > lastVol-volSpan; k--) {
	if(vortexList->at(k).getPressure()==-999)
	  continue;
	recentAv+=vortexList->at(k).getPressure();
	recentCount++;
      }      
      recentAv /= (recentCount*1.);
      float timeSpan = vortexList->at(lastVol-volSpan).getTime().secsTo(vortexList->at(lastVol).getTime());
      QDateTime pastTime = vortexList->at(lastVol).getTime().addSecs(-1*int(timeSpan/2+3600));
      for(int k = 0; k < lastVol; k++) {
	if(vortexList->at(k).getPressure()==-999)
	  continue;
	if(vortexList->at(k).getTime() <= pastTime)
	  pastCenter = k;
      }      
      for(int j = pastCenter-int(volSpan/2.); 
	  (j < pastCenter+int(volSpan/2.))&&(j<lastVol); j++) {
	if(vortexList->at(j).getPressure()==-999)
	  continue;
	pastAv+= vortexList->at(j).getPressure();
	pastCount++;
      }
      pastAv /= (pastCount*1.);
      if(recentAv - pastAv > rapidRate) {
	emit(log(Message(QString("Rapid Increase in Storm Central Pressure Reported @ Rate of "+QString().setNum(recentAv-pastAv)+" mb/hour"), 0,this->objectName(), Green, QString(), RapidIncrease, QString("Storm Pressure Rising"))));
      } else {
	if(recentAv - pastAv < -1.0*rapidRate) {
	  emit(log(Message(QString("Rapid Decline in Storm Central Pressure Reporting @ Rate of "+QString().setNum(recentAv-pastAv)+" mb/hour"), 0, this->objectName(), Green, QString(), RapidDecrease, QString("Storm Pressure Falling"))));
	}
	else {
	  emit(log(Message(QString("Storm Central Pressure Stablized"), 0, this->objectName(),Green,QString(), Ok, QString())));
	}
      }
    }
  }
}
			 

void PollThread::checkListConsistency()
{
  if(vortexList->count()!=simplexList->count()) {
    emit log(Message(QString("Storage Lists Reloaded With Mismatching Volume Entries"),0,this->objectName()));
  }
  
  for(int vv = vortexList->count()-1; vv >= 0; vv--) {
    bool foundMatch = false;
    for(int ss = 0; ss < simplexList->count(); ss++) {
      if(vortexList->at(vv).getTime()==simplexList->at(ss).getTime())
	foundMatch = true;
    }
    if(!foundMatch) {
      emit log(Message(QString("Removing Vortex Entry @ "+vortexList->at(vv).getTime().toString(Qt::ISODate)+" because no matching simplex was found"),0,this->objectName()));
      vortexList->removeAt(vv);
    }
  }

  for(int ss = simplexList->count()-1; ss >= 0; ss--) {
    bool foundMatch = false;
    for(int vv = 0; vv < vortexList->count(); vv++) {
      if(simplexList->at(ss).getTime() == vortexList->at(vv).getTime())
	foundMatch = true;
    }
    if(!foundMatch) {
      emit log(Message(QString("Removing Simplex Entry @ "+simplexList->at(ss).getTime().toString(Qt::ISODate)+" Because No Matching Vortex Was Found"),0,this->objectName()));
      simplexList->removeAt(ss);
    }
  } 
  // Removing the last ones for safety, any partially formed file could do serious damage
  // to data integrity
  simplexList->removeAt(simplexList->count()-1);
  simplexList->save();
  vortexList->removeAt(vortexList->count()-1);
  vortexList->save();
}
  
void PollThread::catchCappiInfo(const float& x, const float& y, 
				const float& rmwEstimate, const float& sMin,const float& sMax,const float& vMax, 
				const float& userLat, const float& userLon, const float& lat, const float& lon)
{
  emit newCappiInfo(x,y,rmwEstimate,sMin,sMax,vMax,userLat,userLon,lat,lon);
}
