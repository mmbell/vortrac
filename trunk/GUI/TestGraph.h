/*
 * TestGraph.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/9/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef TESTGRAPH_H
#define TESTGRAPH_H

#include <QWidget>
#include "DataObjects/VortexData.h"
#include "DataObjects/VortexList.h"

class TestGraph:public QWidget
{
Q_OBJECT
public:
  TestGraph(QWidget *parent = 0);
  void examplePlotNumbers(VortexList *VortexPointer,
			  VortexList *dropPointer, 
			  int number_of_VortexData);
  void listPlotNumbers(QString fileName,VortexList *VortexPointer,
		       VortexList *dropPointer);
  // These function allows us to plot sample data points;
  // All the data points viewed on the graph are generated within this function

  public slots:
    void examplePlot();
    void listPlot(); 

  signals:
  void listChanged(VortexList *glist);
  void dropListChanged(VortexList *gDropList);
};

#endif
