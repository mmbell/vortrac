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

TestGraph::TestGraph(QWidget *parent)
  :QWidget(parent)
{
}


void TestGraph::examplePlot()
{
  QList<VortexData> *globalVortexDataList = new QList<VortexData>();
  QList<VortexData> *globalDropList = new QList<VortexData>();
  examplePlotNumbers(globalVortexDataList, globalDropList,45);
}

//***********************--examplePlotNumbers--*************************************

void TestGraph::examplePlotNumbers(QList<VortexData>* VortexPointer,QList<VortexData> *dropPointer, int number_of_VortexData)
{
  QDateTime first = QDateTime::currentDateTime();
  //uses new info and newDropS to create a lists of points for graphic test purposes
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
