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

    if(skipReadVolume())
      return true;
  }
  else {
    if(readVolumeAnalytic())
      return true;
  }

return false;
}

Sweep AnalyticRadar::addSweep()
{
  /*
  // add Sweeps from theoretical radar sampling
  Sweep newSweep;
  newSweep.setSweepIndex[numSweeps];
  newSweep.setFirstRay[numRays];
  newSweep->setNyquist_vel( nyqVel );
  newSweep->setFirst_ref_gate( );
  newSweep->setFirst_vel_gate( );
  newSweep->setRef_gatesp( refGateSp );
  newSweep->setVel_gatesp( velGateSp );
  newSweep->setVcp( vcp );
  numSweeps++;
  return newSweep;
  */
}

Ray AnalyticRadar::addRay()
{
  /*
  // Add a new ray to the sweep
  Ray *newRay = new Ray();
  newRay->setSweepIndex( (numSweeps - 1) );
  newRay->setRayIndex(numRays-1);
  
  float elevAngle = Sweeps[numSweeps-1].getElevation();
  float azimAngle = beamWidth*(newRay.rayIndex-Sweeps[numSweeps-1].getFirstRay());
  newRay->setAzimuth( azimAngle );
  newRay->setElevation( elevAngle );
  newRay->setNyquist_vel( nyqVel );

  int numPoints = data->getSphericalRangeLength(azimAngle, elevAngle);
  float *raw_ref_data = new float[numPoints];
  float *raw_ref_positions = new float[numPoints];
  raw_ref_data = data->getSphericalRangeLength(QString("RF"),
					       azimAngle, elevAngle);
  raw_ref_positions = data->getSphericalRangePositions(azimAngle, elevAngle);
  
  float furthestPosition = 0;
  for(int z = 0; z < numPoints; z++) {
    if(raw_ref_positions[z] > furthestPosition)
      furthestPosition = raw_ref_positions[z];
  }
  int numRefGates = ceil(furthestPostion/refGateSp);
  float *ref_data = new float[numRefGates];
  float gateBoundary = 0;
  int gateNum = 0;

  While(gateBoundary < furthestPosition) {
    float refSum = 0;
    int count = 0;
    for(int p = 0; p < numPoints; p++) {
      if((raw_ref_positions[p] > gateBoundary) 
	 && (raw_ref_positions[p] < gateBoundary+refGateSp)) {
	refSum+= raw_ref_data[p];
	count++;
      }
    }
    ref_data[gateNum] = refSum/count;
    gateBoundary+= refGateSp;
    gateNum++;
  }
  
  float *raw_vel_data = new float[numPoints];
  

  //newRay->setTime( );
  //newRay->setDate( );
  //newRay->setVelResolution( radarHeader->velocity_resolution );
  //newRay->setUnambig_range( (radarHeader->unamb_range_x10) / 10.0 );
 
  newRay->setRefData( ref_data );
  newRay->setVelData( vel_data );
  //newRay->setSwData( sw_data ); 
  //newRay->setFirst_ref_gate( );
  //newRay->setFirst_vel_gate( );
  newRay->setRef_gatesp( refGateSp );
  newRay->setVel_gatesp( velGateSp );
  newRay->setRef_numgates( numRefGates );
  newRay->setVel_numgates( numVelGates );
  newRay->setVcp( vcp );
  numRays++;
  return newRay;

  */
}


bool AnalyticRadar::skipReadVolume()
{
  // This indicates that none of the RadarQC is used
  // This radarData is a dude, and goes straight to cappi
  
  numSweeps = -1;
  numRays = -1;

  return true;
}

bool AnalyticRadar::readVolumeAnalytic()
{
  /*
    
  // This section creates a griddedData to be sampled by the theoretical
  // radar and go through all the RadarQC
  
  vortexLat = masterRoot.firstChildElement("vortex").firstChildElement("lat").text().toFloat();
  vortexLon = masterRoot.firstChildElement("vortex").firstChildElement("lon").text().toFloat();
  radarLat = config->getParam(config->getConfig("analytic_radar"), 
			      "radarY").toFloat();
  radarLon = config->getParam(config->getConfig("analytic_radar"),
			      "radarX").toFloat();
  nyqVel= config->getParam(config->getConfig("analytic_radar"), 
			   "nyquistVel").toFloat();
  refGateSp = config->getParam(config->getConfig("analytic_radar"),
			       "refgatesp").toFloat();
  velGateSp = config->getParam(config->getConfig("analytic_radar"), 
			       "velgatesp").toFloat();
  beamWidth = config->getParam(config->getConfig("analytic_radar"), 
			       "beamwidth").toFloat();

  radarName = QString("Analytic Radar"); 
  
  GriddedFactory *factory = new GriddedFactory();
  data = factory->makeAnalytic(this, masterRoot.firstChildElement("cappi"), 
			       config, &vortexLat, &vortexLon);
  data->setPointOfInterest((int)radarLon,(int)radarLat,0);
 
  // Sample with theoretical radar
  // VCP 32 

  *Sweeps = addSweep();
  *Rays = addRay()
  for(int r = 1; 
  
  for(int n = 1; n < 5; n++) {
    
  
  }
  */
  return false;
}

