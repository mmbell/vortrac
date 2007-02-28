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

#include "DataObjects/SimplexList.h"

PollThread::PollThread(QObject *parent)
  : QThread(parent)
{
  this->setObjectName("pollThread");
  //  Message::toScreen("PollThread Constructor");
  abort = false;
  runOnce = false;
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
  Message::toScreen("PollThread Destructor IN");
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
  Message::toScreen("PollThread Destructor OUT");
}

void PollThread::setConfig(Configuration *configPtr)
{

  configData = configPtr;

}

void PollThread::abortThread()
{
  Message::toScreen("In PollThread Abort");
  mutex.lock();
  abort = true;
  mutex.unlock();
 
  delete analysisThread;

  //this->exit();
  if(this->isRunning())
    waitForAnalysis.wakeAll();  // What does this do ? -LM
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
  Message::toScreen("Leaving PollThread Abort");
 
}

void PollThread::analysisDoneProcessing()
{
  waitForAnalysis.wakeOne();
}

void PollThread::run()
{

  //Message::toScreen("Begining run sequence in PollThread");
  
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
    connect(vortexConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    vortexList = new VortexList(vortexConfig);
    vortexList->open();
  
    file = QString("vortrac_defaultSimplexListStorage.xml");
    //simplexConfig = new Configuration(0,QString());
    simplexConfig = new Configuration(0, file);
    connect(simplexConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    simplexList = new SimplexList(simplexConfig);
    simplexList->open();
    
    file = QString("vortrac_defaultPressureListStorage.xml");
    //pressureConfig = new Configuration(0,QString());
    pressureConfig = new Configuration(0, file);
    connect(pressureConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    pressureList = new PressureList(pressureConfig);
    pressureList->open();

    dropSondeConfig = new Configuration(0, file);
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
    emit log(Message(file));
    vortexConfig = new Configuration(0, workingDirectory.filePath(file));
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
    emit log(Message(file));
    simplexConfig = new Configuration(0, workingDirectory.filePath(file));
    connect(simplexConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
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
    }
    else {
      file = QString("vortrac_defaultPressureListStorage.xml");
    }
    emit log(Message(file));
    pressureConfig = new Configuration(0, workingDirectoryPath+file);
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
    }
    else {
      file = QString("vortrac_defaultPressureListStorage.xml");
    }
    emit log(Message(file));
    dropSondeConfig = new Configuration(0, workingDirectoryPath+file);
    connect(dropSondeConfig, SIGNAL(log(const Message&)), 
	    this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
    dropSondeList = new PressureList(dropSondeConfig);
    dropSondeList->open();

    outFileName = workingDirectory.path()+"/";
    outFileName += vortexName+"_"+radarName+"_"+year+"_dropSondeList.xml";
    dropSondeList->setFileName(outFileName);
    dropSondeList->setNewWorkingDirectory(workingDirectory.path());

    emit vortexListUpdate(vortexList);

  }  

  //    Message::toScreen("Past list initialization in PollThread");

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
  analysisThread->setVortexList(vortexList);
  analysisThread->setSimplexList(simplexList);
  analysisThread->setPressureList(pressureList);
  analysisThread->setDropSondeList(dropSondeList);

	// Begin polling loop
	forever {

	  //	  Message::toScreen("In forever loop in PollThread");
	  
	  // Check for new data
	  if (dataSource->hasUnprocessedData()) {
	    dataSource->updateDataQueue(vortexList);
	    analysisThread->setNumVolProcessed(dataSource->getNumProcessed());
	    //Message::toScreen("Radar Factory has "+QString().setNum(dataSource->getNumProcessed())+" processed volumes in PollThread");
	    
	    // Fire up the analysis thread to process it
	    RadarData *newVolume = dataSource->getUnprocessedData();
	    if(newVolume == NULL)
	      continue;
	    
	    //emit log(Message(QString(),2,this->objectName()));

	    // Check to makes sure that the file still exists and is readable
	    
	    if(!newVolume->fileIsReadable()) {
	      emit log(Message(QString("The radar data file "+newVolume->getFileName()+" is not readable"), -1,this->objectName()));
	      continue;
	    }

	    analysisThread->analyze(newVolume,configData);
	    
	    mutex.lock();
	    if (!abort) {
	      while(!waitForAnalysis.wait(&mutex, 60000)) {
		//Message::toScreen("Not abort: in pollthread loop");
		// Check for new pressure measurements every minute while we are waiting
		while (pressureSource->hasUnprocessedData()) {
		  PressureList* newObs = pressureSource->getUnprocessedData();
		  for (int i = 0; i < newObs->size(); i++) 
		    pressureList->append(newObs->at(i));
		  delete newObs;
		  // Hopefully, the filename has been set by the analysisThread by this point
		  // have to be careful about synchronization here, this may not be the best way to do this
		  if (!pressureList->getFileName().isNull()) 	
		    pressureList->save();						
		}
		
	      }
	      
	      // Done with radar volume, send a signal to the Graph to update
	      emit vortexListUpdate(vortexList);
	      //Message::toScreen("Wait for analysis done");
	    }
	    mutex.unlock();  
	  }
	  
	  // Check to see if we should quit
	  if (abort) {
	    //Message::toScreen("Abort is true, before return in pollthread");
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
