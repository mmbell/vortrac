/*
 *  AnalyticGrid.h
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 2/11/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef ANALYTICGRID_H
#define ANALYTICGRID_H

#include <QDomElement>
#include <QFile>
#include "DataObjects/GriddedData.h"
#include "Config/Configuration.h"

class AnalyticGrid : public GriddedData
{
 
 public:
  AnalyticGrid();

  ~AnalyticGrid();

  void gridAnalyticData(QDomElement cappiConfig,Configuration* analyticConfig,
			float *vortexLat, float *vortexLon, float *radarLat,
			float *radarLon);

  void gridWindFieldData();
  
  void writeAsi();
  void testRange();
   
 private:
  enum InfoSource {
    mm5,
    windFields
  };

  InfoSource source;

  float centX;
  float centY;
  float radX;
  float radY;
  float rmw;
  float envSpeed;
  float envDir;
  //  float scale;
  QList<float> vT;
  QList<float> vR;
  QList<float> vTAngle;
  QList<float> vRAngle;

  float xmin, xmax;
  float ymin, ymax;
  float zmin, zmax;
  


  float latReference, lonReference;
  QString outFileName;
  float* relDist;
  
  
  
};
               

#endif
