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
 
  abort = false;
  connect(&analysisThread, SIGNAL(doneProcessing()), 
  	  this, SLOT(analysisDoneProcessing()));
  connect(&analysisThread, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  connect(this, SIGNAL(terminated()),
	  &analysisThread, SLOT(terminate()));
  connect(&analysisThread, SIGNAL(newVCP(const int)),
	  this, SLOT(catchVCP(const int)), Qt::DirectConnection);
 
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

void PollThread::analysisDoneProcessing()
{
  waitForAnalysis.wakeOne();
}

void PollThread::run()
{
  emit log(Message("PollThread Started"));
  RadarFactory *dataSource = new RadarFactory(configData->getConfig("radar"));
  connect(dataSource, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);

  //QString file("/scr/science40/mauger/Working/trunk/LisaList.xml");
  QString file("vortrac_defaultVortexListStorage.xml");

  vortexConfig = new Configuration(0, QString());
  //  QString newWorkingDirectory = "/scr/science40/mauger/WorkingDir/trunk/";
  QString newWorkingDirectory = QString("");
  //vortexList->setNewWorkingDirectory(newWorkingDirectory);
  
  //vortexConfig->read(file);
  connect(vortexConfig, SIGNAL(log(const Message&)), 
	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
 
  vortexConfig = new Configuration(0, file);
  
  vortexList = new VortexList(vortexConfig);
  vortexList->open();
  // Testing SimplexList ----------------------------------------------------
  /*
  file = QString("/scr/science40/mauger/Working/trunk/LisaSimplex.xml");

  Configuration *simplexConfig = new Configuration(0,QString());
  connect(simplexConfig, SIGNAL(log(const Message&)), 
	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  simplexConfig = new Configuration(0, file);
  
  SimplexList *list = new SimplexList(simplexConfig);
  list->open();

  Message::toScreen("Num simplex: "+QString().setNum(list->count()));

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
  

  analysisThread.setVortexList(vortexList);

	// Begin polling loop
	forever {

		// Check for new data
		if (dataSource->hasUnprocessedData()) {
			// Got some data, fire up the thread to process it
			analysisThread.analyze(dataSource->getUnprocessedData(),
					       configData);
			mutex.lock();
			if (!abort) {
			  waitForAnalysis.wait(&mutex);
			}
			mutex.unlock();  
		}
		
		// Check to see if we should quit
		if (abort)
			return;
    
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
