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
  Message::toScreen("PollThread Constructor");
  abort = false;
  runOnce = false;
  analysisThread = new AnalysisThread();
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

void PollThread::abortThread()
{
  Message::toScreen("In PollThread Abort");
  if (analysisThread->isRunning()) {
    mutex.lock();
    Message::toScreen("in PollThread Exit");
    analysisThread->abortThread();
    //analysisThread = new AnalysisThread;
    //analysisThread->quit();
    
    //Message::toScreen("PollThread has mutex locked!");
    mutex.unlock();
    
 }
  mutex.lock();
  abort = true;
  mutex.unlock();
  //this->terminate();
  Message::toScreen("Leaving PollThread Abort");
 
}

void PollThread::analysisDoneProcessing()
{
  waitForAnalysis.wakeOne();
}

void PollThread::run()
{
  abort = false;
  emit log(Message("Polling for data..."));
  //Message::toScreen("Breaks connections but gets back to run??");
  RadarFactory *dataSource = new RadarFactory(configData->getConfig("radar"));
  connect(dataSource, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);

  PressureFactory *pressureSource = new PressureFactory(configData);
  connect(pressureSource, SIGNAL(log(const Message&)),
		  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  
  //QString file("/scr/science40/mauger/Working/trunk/LisaList.xml");
  //vortexConfig = new Configuration(0, QString());
  //  QString newWorkingDirectory = "/scr/science40/mauger/WorkingDir/trunk/";
  //QString newWorkingDirectory = QString("");
  //vortexList->setNewWorkingDirectory(newWorkingDirectory);
  //vortexConfig->read(file);
  
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
  dropsondeList = new PressureList(pressureConfig);
  dropsondeList->open();
  
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
  // Testing VortexList ------------------------------------------------------
  //vortexList->value(0).printString();

  
  VortexData test(15,2,2);
  test.setPressure(7);
  test.setPressureUncertainty(7);
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
  analysisThread->setVortexList(vortexList);
  analysisThread->setSimplexList(simplexList);
  analysisThread->setPressureList(pressureList);

	// Begin polling loop
	forever {

		// Check for new data
		if (dataSource->hasUnprocessedData()) {

			// Fire up the analysis thread to process it
			analysisThread->analyze(dataSource->getUnprocessedData(),
					       configData);
			//Message::toScreen("Before mutex lock in pollThread");
			
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
			  Message::toScreen("Wait for analysis done");
			}
			mutex.unlock();  
			//Message::toScreen("After mutex unlock in pollThread");
		}
		
		// Check to see if we should quit
		if (abort) {
		  //Message::toScreen("Abort is true, before return in pollthread");
		  return;
		}
		if (runOnce) {
		  emit log(Message(tr("Analysis Completed Exiting PollThread"),
				   -1));
		  runOnce = false;
		  return;
		}
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


void PollThread::catchVCP(const int vcp)
{
  emit newVCP(vcp);
}

void PollThread::setOnlyRunOnce(const bool newRunOnce) {
  mutex.lock();
  runOnce = newRunOnce;
  mutex.unlock();
}
