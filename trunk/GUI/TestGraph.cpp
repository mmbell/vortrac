/*
 * TestGraph.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/9/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "TestGraph.h"
#include "Configuration.h"
#include <QDomNodeList>

TestGraph::TestGraph(QWidget *parent)
  :QWidget(parent)
{
}


void TestGraph::examplePlot()
{
  // This parameter-less publics slot was created for connecting
  // to the button on screen that will launch the generic plot.

  QList<VortexData> *globalVortexDataList = new QList<VortexData>();
  QList<VortexData> *globalDropList = new QList<VortexData>();
  examplePlotNumbers(globalVortexDataList, globalDropList,45);
}

void TestGraph::examplePlotNumbers(QList<VortexData>* VortexPointer,
				   QList<VortexData> *dropPointer, 
				   int number_of_VortexData)
{

  // Plots artificial data onto the graphFace plot through interacting
  // with the newInfo and newDrop slots and signals

  QDateTime first = QDateTime::currentDateTime();
  for(int i = 0; i < number_of_VortexData;i++)
    {
      VortexData *temp = new VortexData();
      temp->setPressure(980-(.5*i));
      temp->setRMW(0, 10+(.125*i));
      temp->setTime(first.addSecs(600*i));
      temp->setPressureUncertainty(1.5-.025*i);
      temp->setRMWUncertainty( 0, .025*i);
      VortexPointer->append(*temp);
		
      emit listChanged(VortexPointer);
      delete temp;
    }
  
  for(int i = 0; i<(number_of_VortexData/3); i++)
    {
      VortexData *temp = new VortexData();
      temp->setPressure( 980-(1.5*i));
      temp->setTime(first.addSecs(900*i));
	
      dropPointer->append(*temp);
      emit dropListChanged(dropPointer);
      delete temp;
	
    }
  
}

void TestGraph::listPlot()
{
  // This public slot was created specifically for launching the 
  // listPlotNumbers function. This file contains data from 
  // huricane Katrina

  QList<VortexData> *globalVortexDataList = new QList<VortexData>();
  QList<VortexData> *globalDropList = new QList<VortexData>();
  QString file(QString("/scr/science40/mauger/Working/trunk/vortrac_testVortexData.xml"));
  listPlotNumbers(file, globalVortexDataList, globalDropList);
}

void TestGraph::listPlotNumbers(QString fileName,
				QList<VortexData> *VortexPointer,
				QList<VortexData> *dropPointer)
{
  
  // Creates a configuration to read in the data and retreive appropriate
  // values to populate the vortexData list, for a test display

  Configuration *vortexPoints = new Configuration(this, fileName);
  vortexPoints->read(fileName);
  QDomElement current = vortexPoints->getRoot().firstChildElement();
  while(!current.isNull()) {
    if(current.tagName()!=QString("vortex")) {
      QString timeString = vortexPoints->getParam(current, QString("time"));
      QDateTime time = QDateTime::fromString(timeString, Qt::ISODate);
      float pressure = vortexPoints->getParam(current, 
					      QString("pressure")).toFloat();
      float pressureUn = vortexPoints->getParam(current,
				    QString("pressure_uncertainty")).toFloat();
      float rmw = vortexPoints->getParam(current, 
					 QString("rmw")).toFloat();
      float rmwUn = vortexPoints->getParam(current,
			   QString("rmw_uncertainty")).toFloat();
      
      VortexData *temp = new VortexData();
      temp->setPressure(pressure);
      temp->setRMW(0, rmw);
      temp->setTime(time);
      temp->setPressureUncertainty(pressureUn);
      temp->setRMWUncertainty( 0, rmwUn);
      VortexPointer->append(*temp);
		
      emit listChanged(VortexPointer);
      delete temp;
      
    }
    current = current.nextSiblingElement();
  }
  
  VortexData *drop = new VortexData();
  drop->setPressure(917);
  drop->setTime(QDateTime::fromString(QString("2005-08-29T09:00:00"), 
				       Qt::ISODate));
  // Actually Happened at 9am

  dropPointer->append(*drop);
  emit dropListChanged(dropPointer);
  
  drop = new VortexData();
  drop->setPressure(918);
  drop->setTime(QDateTime::fromString(QString("2005-08-29T10:00:00"), 
				      Qt::ISODate));
  
  // Actually Happened at 10am

  dropPointer->append(*drop);
  emit dropListChanged(dropPointer);

  drop = new VortexData();
  drop->setPressure(920);
  drop->setTime(QDateTime::fromString(QString("2005-08-29T11:00:00"), 
				       Qt::ISODate));
  
  // Actually Happened at 11am

  dropPointer->append(*drop);
  emit dropListChanged(dropPointer);
  

  delete (drop);
}
