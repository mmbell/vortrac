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
  this->setObjectName("pollThread");
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
  emit log(Message(QString("INTO Destructor"),0,this->objectName()));
  mutex.lock();
  abort = true;
  mutex.unlock();
  //  if(this->isRunning())
  this->abortThread();
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
    delete dropSondeList;
    delete vortexConfig;  // This should be deleted do we need to save?
    delete simplexConfig;  // This should be deleted do we need to save?
    delete pressureConfig;   // This should be deleted do we need to save?
    delete dropSondeConfig;   // This should be deleted do we need to save?
    }
  */
  emit log(Message(QString("OUT OF Destructor"),0,this->objectName()));
}

void PollThread::setConfig(Configuration *configPtr)
{

  configData = configPtr;

}

void PollThread::abortThread()
{
  emit log(Message(QString("Enter ABORT"),0,this->objectName()));
  mutex.lock();
  abort = true;
  mutex.unlock();
 
  delete analysisThread;

  //this->exit();
  if(this->isRunning()) {
    waitForAnalysis.wakeAll();  // What does this do ? -LM
    waitForInitialization.wakeAll();
  }
  wait();

  // Deleting Members
  configData = NULL;
  delete configData;
  delete vortexList;
  delete simplexList;
  delete pressureList;
  delete dropSondeList;
  // delete vortexConfig;  // This should be deleted do we need to save?
  // delete simplexConfig;  // This should be deleted do we need to save?
  // delete pressureConfig;   // This should be deleted do we need to save?
  // delete dropSondeConfig;   // This should be deleted do we need to save?
  emit log(Message(QString("Leaving Abort"),0,this->objectName()));
 
}

void PollThread::analysisDoneProcessing()
{
  waitForAnalysis.wakeOne();
  processPressureData = false;
}

void PollThread::run()
{

  abort = false;
  emit log(Message(QString("Polling for data..."), 0, this->objectName()));
  dataSource = new RadarFactory(configData);
  connect(dataSource, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);

  PressureFactory *pressureSource = new PressureFactory(configData);
  connect(pressureSource, SIGNAL(log(const Message&)),
		  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  
  if(!continuePreviousRun) {
    QString file("vortrac_defaultVortexListStorage.xml");
    vortexConfig = new Configuration(0, file);
    vortexConfig->setObjectName("vortexConfig");
    vortexConfig->setLogChanges(false);
    connect(vortexConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    vortexList = new VortexList(vortexConfig);
    vortexList->open();
  
    file = QString("vortrac_defaultSimplexListStorage.xml");
    //simplexConfig = new Configuration(0,QString());
    simplexConfig = new Configuration(0, file);
    simplexConfig->setObjectName("simplexConfig");
    simplexConfig->setLogChanges(false);
    connect(simplexConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    simplexList = new SimplexList(simplexConfig);
    simplexList->open();
    
    file = QString("vortrac_defaultPressureListStorage.xml");
    //pressureConfig = new Configuration(0,QString());
    pressureConfig = new Configuration(0, file);
    pressureConfig->setObjectName("pressureConfig");
    pressureConfig->setLogChanges(false);
    connect(pressureConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    pressureList = new PressureList(pressureConfig);
    pressureList->open();

    dropSondeConfig = new Configuration(0, file);
    dropSondeConfig->setObjectName("dropSondeConfig");
    dropSondeConfig->setLogChanges(false);
    connect(dropSondeConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
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
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
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
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    simplexList = new SimplexList(simplexConfig);
    
    simplexList->open();
    /*
    // Testing simplex List
    Configuration* newSimplexConfig = new Configuration(0,QString("/scr/science40/mauger/Working/trunk/vortrac_defaultSimplexListStorage.xml"));
    SimplexList* testSimplexList = new SimplexList(newSimplexConfig);
    QString simplexPath = configData->getParam(configData->getConfig("center"),
					       "dir");
    QDir simplexWorkingDirectory(simplexPath);
    simplexWorkingDirectory.cdUp();
    simplexWorkingDirectory.cd("centercopy");
    outFileName = workingDirectory.path() + "/";
    outFileName += vortexName+"_"+radarName+"_"+year+"_simplexCOPYList.xml";
    testSimplexList->setFileName(outFileName);
    testSimplexList->setNewWorkingDirectory(simplexWorkingDirectory.path()+"/");
    for(int i = 0; i < simplexList->count(); i++) {
      testSimplexList->append(simplexList->value(i));
    }
    testSimplexList->save();
    QString endOfTest("FINISHED TESTING SIMPLEX");
    Message::toScreen(endOfTest);
    emit log(Message(endOfTest));
    return;
    
    // end simplex list testing
    */

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
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    pressureList = new PressureList(pressureConfig);
    pressureList->open();
    
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
    emit log(Message(file));
    dropSondeConfig->setObjectName("dropSondeConfig");
    dropSondeConfig->setLogChanges(false);
    connect(dropSondeConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    dropSondeList = new PressureList(dropSondeConfig);
    dropSondeList->open();

    outFileName = workingDirectory.path()+"/";
    outFileName += vortexName+"_"+radarName+"_"+year+"_dropSondeList.xml";
    dropSondeList->setFileName(outFileName);
    dropSondeList->setNewWorkingDirectory(workingDirectory.path());

    checkListConsistency();

    emit vortexListUpdate(vortexList);

  }  

          //Message::toScreen("Num simplex: "+QString().setNum(list->count()));
          /* Fake simplex list data
	     SimplexData newData(15,6,2);
	     newData.setTime(QDateTime::currentDateTime());
	     for(int i = 0; i < 2; i++) {
	     newData.setX(i,0,7);
	     newData.setY(i,0,7);
	     newData.setHeight(i,7);
	     newData.setCenterStdDev(i,0,7);
	     newData.setMaxVT(i,0,7);
	     newData.setVTUncertainty(i,0,7);
	     newData.setNumConvergingCenters(i,0,7);
	     Center newCenter(8,8,8,i,0);
	     newData.setCenter(i,0,0,newCenter);
	     Center otherCenter(9,9,9,i,0);
	     newData.setCenter(i,0,1,otherCenter);
	     }
	     //newData.printString();
	     // list->append(newData);
	     Message::toScreen("Num simplex: "+QString().setNum(list->count()));
	     list->value(0).printString();
	     list->value(1).printString();
	     file = QString("/scr/science40/mauger/Working/trunk/LisaSimplex.xml");
	     list->setFileName(file);
	     list->save();
	  */
  
  //-------------------------------------------------------------------------
  
          /*
	  // Testing VortexList ---------------------------------------
	  //vortexList->value(0).printString();
	  
	  
	  VortexData test(15,2,2);
	  test.setPressure(7);
	  test.setPressureUncertainty(7);  if(pollThread->isNull())
	  return false;
	  test.setTime(QDateTime::currentDateTime());
	  for(int i = 0; i < 3; i++ ){
	  test.setLat(i,7.0);
	  test.setLon(i,7.0);
	  test.setAltitude(i,7.0);
	  test.setRMW(i, 7.0);
	  test.setRMWUncertainty(i, 7.0);
	  test.setNumConvergingCenters(i, 7);
	  test.setCenterStdDev(i,7.0);
	  Coefficient coeff(6,6,6,"VTCO");
	  Coefficient coeff2(5,5,5,"VRCO");
	  test.setCoefficient(i,0,0,coeff);
	  test.setCoefficient(i,1,0,coeff2);
	  }
	  
	  test.printString();
	  
	  vortexList->append(test);
	  
	  Message::toScreen("next Count: "+QString().setNum(vortexList->count()));
	  
	  // vortexList->timeSort();
	  //vortexList->value(0).printString();
	  
	  QString saveFile("/scr/science40/mauger/Working/trunk/LisaList.xml");
	  
	  vortexList->setFileName(saveFile);
	  
	  vortexList->save();
	  */
  //--------------------------------------------------------------------------
  
  analysisThread = new AnalysisThread;
  connect(analysisThread, SIGNAL(doneProcessing()), 
  	  this, SLOT(analysisDoneProcessing()));
  connect(analysisThread, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  connect(this, SIGNAL(terminated()),
	  analysisThread, SLOT(terminate()));
  connect(analysisThread, SIGNAL(newVCP(const int)),
	  this, SLOT(catchVCP(const int)), Qt::DirectConnection);
  connect(analysisThread, SIGNAL(newCappi(const GriddedData*)),
	  this, SLOT(catchCappi(const GriddedData*)), Qt::DirectConnection);
  connect(analysisThread, SIGNAL(finishedInitialization()), this, SLOT(initializationComplete()), 
	  Qt::DirectConnection);
  analysisThread->setVortexList(vortexList);
  analysisThread->setSimplexList(simplexList);
  analysisThread->setPressureList(pressureList);
  analysisThread->setDropSondeList(dropSondeList);
  analysisThread->setAnalyticRun(runOnce);


	// Begin polling loop
	forever {

     	  // Check for new data
	  if (dataSource->hasUnprocessedData()) {
	    mutex.lock();
	    dataSource->updateDataQueue(vortexList);
	    analysisThread->setNumVolProcessed(dataSource->getNumProcessed());
	    
	    // Fire up the analysis thread to process it
	    RadarData *newVolume = dataSource->getUnprocessedData();
	    if(newVolume == NULL)
	      continue;
	    
	    //emit log(Message(QString(),2,this->objectName()));

	    // Check to makes sure that the file still exists and is readable
	    
	    if(!newVolume->fileIsReadable()) {
	      emit log(Message(QString("The radar data file "+newVolume->getFileName()+" is not readable"), -1,this->objectName()));
	      mutex.unlock();
	      continue;
	    }
	    mutex.unlock();
	 
	    analysisThread->analyze(newVolume,configData);
	    
	    mutex.lock();

	    // Insure that the list are initialized
	    waitForInitialization.wait(&mutex);
	    QString radarFileName = newVolume->getFileName();

	    if (!abort) {
	      
	      // Check for new pressure measurements every minute while we are waiting
	      while (pressureSource->hasUnprocessedData()
		     && processPressureData) {
			
		QList<PressureData>* newObs = pressureSource->getUnprocessedData();
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
		/*

		// Moving this step to later so we know that analysisThread is done
		// This should only present a problem if we crash mid run, then the pressure obs
		// Will not be in the list, but if we crash I think this is the least of our problems -LM
		
		// Hopefully, the filename has been set by the analysisThread by this point
		// have to be careful about synchronization here, this may not be the best way to do this
		if (!pressureList->getFileName().isNull()) 	
		pressureList->save();
		*/
		
	      }
	      
	      if(!processPressureData || waitForAnalysis.wait(&mutex))
		processPressureData = true;
	           
	      // Saving the new data entries
	      if (!pressureList->getFileName().isNull()) 	
		pressureList->save();
	      
	      // Done with radar volume, send a signal to the Graph to update
	      emit vortexListUpdate(vortexList);
	      emit log(Message(QString("Completed Analysis On Volume "+radarFileName),100,
			       this->objectName(),Green));
	    }
	    mutex.unlock();  
	  }
	  
	  checkIntensification();
	  
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
	
	emit log(Message("PollThread Finished"));	
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

  // Make this an input parameter
  //float rapidRate = 3; // Units of mb per hr
  float rapidRate = configData->getParam(pressure, QString("rapidlimit")).toFloat();
  if(isnan(rapidRate)) {
    emit log(Message(QString("Could Not Find Rapid Intensification Rate, Using 3 mb/sec"),
		     0,this->objectName()));
    rapidRate = 3.0;
  }
  
  // So we don't report falsely there must be a rapid increase trend which 
  // spans several measurements

  //  int volSpan = 8;  // Number of volumes which are averaged.
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
      for(int k = lastVol; k >= lastVol-volSpan; k--) {
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
	if(vortexList->at(k).getTime() > pastTime)
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
	emit(log(Message(QString("Rapid Intensification Reported @ Rate of "+QString().setNum(recentAv-pastAv)+" mb/hour"), 0,this->objectName(), Green, QString(), RapidIncrease, QString("Storm Pressure Rising"))));
      } else {
	if(recentAv - pastAv < -1*rapidRate) {
	  emit(log(Message(QString("Rapid Decline in Storm Central Pressure Reporting @ Rate of "+QString().setNum(recentAv-pastAv)+" mb/hour"), 0, this->objectName(), Green, QString(), RapidDecrease, QString("Storm Pressure Dropping"))));
	}
	else {
	  emit(log(Message(QString("Storm Central Pressure Stablized"), 0, this->objectName(),Green, 
			   QString(), Ok, QString())));
	}
      }
    }
  }
}

void PollThread::initializationComplete()
{
  waitForInitialization.wakeAll();
}

void PollThread::checkListConsistency()
{
  if(vortexList->count()!=simplexList->count()) {
    emit log(Message(QString("Storage Lists Reloaded With Mismatching Volume Entries")));
  }
  for(int vv = 0; vv < vortexList->count(); vv++) {
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

  for(int ss = 0; ss < simplexList->count(); ss++) {
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
  vortexList->removeAt(vortexList->count()-1);
}
  
