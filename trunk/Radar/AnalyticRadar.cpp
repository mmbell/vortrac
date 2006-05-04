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
#include "IO/Message.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

 AnalyticRadar::AnalyticRadar(const QString& radarname, float lat,
			      float lon, const QString& configFile)
   : RadarData(radarname, lat, lon, configFile)
   
{

  config = new Configuration();
  config->read(configFile);
  QDomElement radar = config->getRoot().firstChildElement("analytic_radar");
  QString sampling = config->getParam(radar, "sample");
  
  if (sampling == QString("false") || sampling == QString("no")) {
    isDealiased(true);
  }
  else { 
    //Message::toScreen("Analytic Radar Not Dealaised");
    Message::toScreen("Sampling Analytic Storm with Analytic Radar");
    isDealiased(false);
  }

  numSweeps = 0;
  numRays = 0;
  vcp = 0;

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
    if(readVolumeAnalytic()) {
      //testing Message::toScreen("Finished read Volume Analytic");
      return true;
    }
  }

return false;
}

Sweep AnalyticRadar::addSweep()
{
  
  // add Sweeps from theoretical radar sampling
  Sweep *newSweep = new Sweep();

  // Needs a way to do different elevation angles -LM
  newSweep->setElevation(elevations[numSweeps]);
  newSweep->setSweepIndex(numSweeps);
  newSweep->setFirstRay(numRays);
  newSweep->setNyquist_vel( nyqVel );
  // newSweep->setFirst_ref_gate( );
  // newSweep->setFirst_vel_gate( );
  newSweep->setRef_gatesp( refGateSp );
  newSweep->setVel_gatesp( velGateSp );
  newSweep->setVcp( vcp );
  numSweeps++;
  return *newSweep;
  
}

Ray AnalyticRadar::addRay()
{
  
  // Add a new ray to the sweep
  Ray *newRay = new Ray();
  newRay->setSweepIndex( (numSweeps - 1) );
  newRay->setRayIndex(numRays);
  
  float elevAngle = Sweeps[numSweeps-1].getElevation();

  float metAzimAngle =  beamWidth*(float)(newRay->getRayIndex()-Sweeps[numSweeps-1].getFirstRay());
  
  float azimAngle = 450.0-metAzimAngle;
  if(azimAngle >= 360.0)
    azimAngle-=360.0;
  newRay->setAzimuth( metAzimAngle );
  newRay->setElevation( elevAngle );
  newRay->setNyquist_vel( nyqVel );

  int numPoints = data->getSphericalRangeLength(azimAngle, elevAngle);
  float *raw_ref_data = new float[numPoints];
  float *raw_ref_positions = new float[numPoints];
  QString dataType("DZ");
  raw_ref_data = data->getSphericalRangeData(dataType,
					     azimAngle, elevAngle);
  raw_ref_positions = data->getSphericalRangePosition(azimAngle, elevAngle);
  
  float furthestPosition = 0.0;
  for(int z = 0; z < numPoints; z++) {
    if(raw_ref_positions[z] > furthestPosition)
      furthestPosition = raw_ref_positions[z];
  }

  //int numRefGates = (int)ceil(furthestPosition/refGateSp);
  
  // numRefGates = 100;
  float *ref_data = new float[numRefGates];
  float gateBoundary = 0;
  
  for(int gateNum = 0; gateNum < numRefGates; gateNum++ ) {
    if(gateBoundary < furthestPosition) {
      float refSum = 0;
      int count = 0;
      for(int p = 0; p < numPoints; p++) {
	if((raw_ref_positions[p] > gateBoundary) 
	   && (raw_ref_positions[p] <= (gateBoundary+refGateSp))) {
	  refSum+= raw_ref_data[p];
	  count++;
	}
      }
      if(count!=0)
	ref_data[gateNum] = refSum/(float)count;
      else
	ref_data[gateNum] = -999;
    }
    else {
      ref_data[gateNum] = -999;
    }
    gateBoundary+=refGateSp;
  }
  
  
  float *raw_vel_data = new float[numPoints];
  dataType = QString("VE");
  raw_vel_data = data->getSphericalRangeData(dataType,azimAngle, elevAngle);
  //int numVelGates = (int)ceil(furthestPosition/velGateSp);
  // int gateNum = 0;
  // int numVelGates = 100;
  float *vel_data = new float[numVelGates];
  float *sw_data = new float[numVelGates];
  gateBoundary = 0;
  
  for(int gateNum = 0; gateNum < numVelGates; gateNum++ ) {
    if(gateBoundary < furthestPosition) {
      float velSum = 0;
      int count = 0;
      for(int p = 0; p < numPoints; p++) {
	if ((raw_ref_positions[p] > gateBoundary) 
	    && (raw_ref_positions[p] <= (gateBoundary+velGateSp))) {
	  velSum += raw_vel_data[p];
	  count++;
	}
      }
      if(count == 0) {
	vel_data[gateNum] = -999;
	sw_data[gateNum] = -999;
      }
      else {
	
	vel_data[gateNum] = velSum*cos(elevAngle*deg2rad)/(float)count;

	// Add in noise factor, method borrowed from analyticTC
	
	srand(time(NULL));  // initializes random number generator
	float noise = rand()%1000/1000.0 -.5;
	vel_data[gateNum]+= scale*noise;

	float *sw_points = new float[count];
	int countAgain = 0;
	for(int p = 0; p < numPoints; p++) {
	  if ((raw_ref_positions[p] > gateBoundary) 
	      && (raw_ref_positions[p] < gateBoundary+velGateSp)) {
	    sw_points[countAgain] = raw_vel_data[p];
	    countAgain++;
	  }
	}
	float swSum = 0;
	for(int q = 0; q < count; q++) {
	  swSum = (sw_points[q]-vel_data[gateNum])*(sw_points[q]-vel_data[gateNum]);
	}
	sw_data[gateNum] = sqrt(swSum/(float)count);

       	while(fabs(vel_data[gateNum]) > nyqVel) {
	  if(vel_data[gateNum] > 0) {
	    vel_data[gateNum]-=2*nyqVel;
	  }
	  else {
	    vel_data[gateNum]+=2*nyqVel;
	  }
       	}
      }
    }
    else {
      vel_data[gateNum] = -999;
      sw_data[gateNum] = -999;
    } 
    gateBoundary+=velGateSp;
  }
    
  newRay->setRefData( ref_data );
  newRay->setVelData( vel_data );
  newRay->setSwData( sw_data );
  newRay->setRef_numgates( numRefGates );
  newRay->setVel_numgates( numVelGates ); 
  newRay->setRef_gatesp( 1000*refGateSp );
  newRay->setVel_gatesp( 1000*velGateSp );  
 
  //newRay->setTi<me( );
  //newRay->setDate( );
 
  // Need more info about how these values are used before filling
  // With Useful info

  //newRay->setVelResolution( radarHeader->velocity_resolution );
  newRay->setUnambig_range( numVelGates*velGateSp);
  newRay->setFirst_ref_gate(0);
  newRay->setFirst_vel_gate(0);

  newRay->setVcp( vcp );
  numRays++;
  return *newRay;
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

  // This section creates a griddedData to be sampled by the theoretical
  // radar and go through all the RadarQC
  
  vortexLat = masterRoot.firstChildElement("vortex").firstChildElement("lat").text().toFloat();
  vortexLon = masterRoot.firstChildElement("vortex").firstChildElement("lon").text().toFloat();


  QDomElement radar = config->getRoot().firstChildElement("analytic_radar");

  radarLat =  masterRoot.firstChildElement("radar").firstChildElement("lat").text().toFloat();
  radarLon =  masterRoot.firstChildElement("radar").firstChildElement("lon").text().toFloat(); 
 
  //  radarLat = config->getParam(radar, "radarY").toFloat();
  //  radarLon = config->getParam(radar, "radarX").toFloat();
  nyqVel= config->getParam(radar, "nyquistVel").toFloat();

  refGateSp = config->getParam(radar, "refgatesp").toFloat();
  velGateSp = config->getParam(radar, "velgatesp").toFloat();
  beamWidth = config->getParam(radar, "beamwidth").toFloat();
  scale = config->getParam(radar, "noise").toFloat();
  float numGates = config->getParam(radar, "numgates").toInt();
  int totNumSweeps = config->getParam(radar, "numsweeps").toInt();
  
  QString sendToDealias = config->getParam(radar, "dealiasdata");
  if((sendToDealias == "true")||(sendToDealias == "True")
     ||(sendToDealias=="TRUE")){
    isDealiased(false);
  }
  else
    isDealiased(true);

  numRefGates = numGates;
  numVelGates = numGates;
  
  radarDateTime = QDateTime::currentDateTime();

  radarName = QString("Analytic Radar"); 
  
  GriddedFactory *factory = new GriddedFactory();
  data = factory->makeAnalytic(this, masterRoot.firstChildElement("cappi"), 
			       config, &vortexLat, &vortexLon, 
			       &radarLat, &radarLon);
  data->writeAsi();
  data->setAbsoluteReferencePoint(radarLat,radarLon,0);
  // data->setCartesianReferencePoint(0,0,0);
 
  // Sample with theoretical radar
  // VCP 0 : flat cappi sample

  vcp = 32;

  int numRaysPerSweep = (int)floor(360.0/beamWidth);
  //testing Message::toScreen(QString().setNum(totNumSweeps*numRaysPerSweep));

  elevations = new float[totNumSweeps];

  elevations[0]  = 0;
  if(totNumSweeps > 1) {
    elevations[1] = 0.5;
    if(totNumSweeps > 2) {
      elevations[1] = 1.5;
      if(totNumSweeps > 3) {
	elevations[2] = 2.5;
	if(totNumSweeps > 4) {
	  elevations[3] = 3.5;
	  if(totNumSweeps > 5) {
	    elevations[4] = 4.5;
	  }}}}}
  
  Sweeps = new Sweep[totNumSweeps];
  Rays = new Ray[totNumSweeps*numRaysPerSweep]; 

  for(int n = 0; n < totNumSweeps; n++) {
    //testing Message::toScreen("Trying to Make Sweep Number "+QString().setNum(n));
    Message::toScreen("Made Sweep "+QString().setNum(n)+" of "+QString().setNum(totNumSweeps-1));
    Sweeps[numSweeps] = addSweep();
    for( int r = 0; r < numRaysPerSweep; r++) {
      // Message::toScreen("Trying to Make Ray Number "+QString().setNum(r));
      Rays[numRays] = addRay();
    }  
    Sweeps[n].setLastRay(numRays-1);
  }

  return true;
}

