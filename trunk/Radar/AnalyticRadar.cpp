/*
 *  AnalyticRadar.cpp
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 02/07/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "AnalyticRadar.h"

 AnalyticRadar::AnalyticRadar(const QString& radarname, float lat,
			      float lon, const QString& configFile)
   : RadarData(radarname, lat, lon, configFile)
   
{
  config = new Configuration();
  config->read(configFile);
  QDomElement radar = config->getRoot().firstChildElement("analytic_radar");
  QString sampling = config->getParam(radar, "sample");
  
  if (sampling == QString("false") || sampling == QString("no"))
    isDealiased(true);
  else 
    isDealiased(false);

  numSweeps = 0;
  numRays = 0;

  // Configure Properties from theoretical radar

}

AnalyticRadar::~AnalyticRadar()
{

}

void AnalyticRadar::setConfigElement(QDomElement configRoot)
{
  masterRoot = configRoot;
}

bool AnalyticRadar::readVolume()
{
  if(isDealiased()) {
    // This indicates that none of the RadarQC is used
    // This radarData is a dude, and goes straight to cappi
    
    numSweeps = -1;
    numRays = -1;

    return true;
  }
  else {
    // This section creates a griddedData to be sampled by the theoretical
    // radar and go through all the radarQC
    
    vortexLat = masterRoot.firstChildElement("vortex").firstChildElement("lat").text().toFloat();
    vortexLon = masterRoot.firstChildElement("vortex").firstChildElement("lon").text().toFloat();

    GriddedFactory *factory = new GriddedFactory();
    data = factory->makeAnalytic(this, masterRoot.firstChildElement("cappi"), 
				 config, &vortexLat, &vortexLon);
    //delete factory;
    // Sample with theoretical radar
    return false;
  }
return false;
}

Sweep AnalyticRadar::addSweep()
{
  // add Sweeps from theoretical radar sampling
  Sweep newSweep;
  return newSweep;
}

Ray AnalyticRadar::addRay()
{
  // add Rays from theoretical radar sampling
  Ray newRay;
  return newRay;
}
