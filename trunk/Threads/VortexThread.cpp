/*
 *  VortexThread.cpp
 *  vortrac
 *
 *  Created by Michael Bell on 2/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <QtGui>
#include <math.h>
#include "VortexThread.h"
#include "DataObjects/Coefficient.h"
#include "DataObjects/Center.h"
#include "VTD/GBVTD.h"
#include "Math/Matrix.h"

VortexThread::VortexThread(QObject *parent)
  : QThread(parent)
{

	  // Initialize RhoBar for pressure calculations (units are Pascal/m)
	  rhoBar[0] = 10.672;
	  rhoBar[1] = 9.703; 
	  rhoBar[2] = 8.792;
	  rhoBar[3] = 7.955;
	  rhoBar[4] = 7.183;
	  rhoBar[5] = 6.467;  
	  rhoBar[6] = 5.817; 
	  rhoBar[7] = 5.227; 
	  rhoBar[8] = 4.689;
	  rhoBar[9] = 4.207;
	  rhoBar[10] = 3.8;
	  rhoBar[11] = 3.3;
	  rhoBar[12] = 2.9;
	  rhoBar[13] = 2.6;
	  rhoBar[14] = 2.2;
	  rhoBar[15] = 1.8;
	  	  
	  abort = false;

	  // Claim and empty all the memory set aside
	  gridData = NULL;
	  vortexData = NULL;
	  pressureList = NULL;
	  configData = NULL;
	  refLat = NULL;
	  refLon = NULL;
	  dataGaps = NULL;
	  //vtd = NULL;       // Don't need these two completely handled 
	  //vtdCoeffs = NULL; // in loop
	  pressureDeficit = NULL;
}

VortexThread::~VortexThread()
{
  mutex.lock();
  abort = true;
  waitForData.wakeOne();
  mutex.unlock();

  gridData = NULL;
  vortexData = NULL;
  pressureList = NULL;
  configData = NULL;
  refLat = NULL;
  refLon = NULL;
  dataGaps = NULL;
  // vtd = NULL;      // Don't need these two completely handled in 
  // vtdCoeffs = NULL;// loop
  pressureDeficit = NULL;
  
  delete gridData;
  delete vortexData;
  delete pressureList;
  delete configData;

  delete [] dataGaps;
  delete [] refLat;
  delete [] refLon;
  delete [] pressureDeficit;
  
  // Wait for the thread to finish running if it is still processing
  wait();

}

void VortexThread::getWinds(Configuration *wholeConfig, GriddedData *dataPtr, VortexData* vortexPtr, PressureList *pressurePtr)
{

	// Lock the thread
	QMutexLocker locker(&mutex);

	// Set the pressure List
	pressureList = pressurePtr;
	
	// Set the grid object
	gridData = dataPtr;

	// Set the vortex data object
	vortexData = vortexPtr;
	
	// Set the configuration info
	configData = wholeConfig;
	vtdConfig = wholeConfig->getConfig("vtd");
      
	// Start or wake the thread
	if(!isRunning()) {
		start();
	} else {
		waitForData.wakeOne();
	}
}

void VortexThread::run()
{
	emit log(Message("VortexThread Started"));
  
	forever {
		// Check to see if we should quit
		if (abort)
		  return;

		// OK, Let's find a center
		mutex.lock();
		bool foundWinds = true;
		
		// Initialize variables
		QString vortexPath = configData->getParam(vtdConfig, 
							   QString("dir"));
		QString geometry = configData->getParam(vtdConfig,
							QString("geometry"));
		QString refField =  configData->getParam(vtdConfig,
				            QString("reflectivity"));
		QString velField = configData->getParam(vtdConfig,
							QString("velocity"));
		QString closure = configData->getParam(vtdConfig,
						       QString("closure"));
		
		firstLevel = configData->getParam(vtdConfig,
					     QString("bottomlevel")).toFloat();
		lastLevel = configData->getParam(vtdConfig,
					     QString("toplevel")).toFloat();
		firstRing = configData->getParam(vtdConfig,
					     QString("innerradius")).toFloat();
		lastRing = configData->getParam(vtdConfig, 
				       	     QString("outerradius")).toFloat();
				
		float ringWidth = configData->getParam(vtdConfig, 
					 QString("ringwidth")).toFloat();
		int maxWave = configData->getParam(vtdConfig, 
					 QString("maxwavenumber")).toInt();
		
		// Set the output directory??
		//vortexResults.setNewWorkingDirectory(vortexPath);
		
		// Define the maximum allowable data gaps
		dataGaps = new float[maxWave+1];
		for (int i = 0; i <= maxWave; i++) {
		  dataGaps[i] = configData->getParam(vtdConfig, 
			QString("maxdatagap"), QString("wavenum"), 
			QString().setNum(i)).toFloat();
		}
		float maxCoeffs = maxWave*2 + 3;
		
		// Create a GBVTD object to process the rings
		vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
		vtdCoeffs = new Coefficient[20];
	
		// Placeholders for centers
		float xCenter = -999;
		float yCenter = -999;
		
		mutex.unlock();
		// Loop through the levels and rings
		for (float height = firstLevel; height <= lastLevel; height++) {
			mutex.lock();
			// Set the reference point
			float refLat = vortexData->getLat(int(height-firstLevel));
			float refLon = vortexData->getLon(int(height-firstLevel));
			gridData->setAbsoluteReferencePoint(refLat, refLon, height);
			if ((gridData->getRefPointI() < 0) || 
				(gridData->getRefPointJ() < 0) ||
				(gridData->getRefPointK() < 0)) {
				// Out of bounds problem
				emit log(Message("Simplex center is outside CAPPI"));
				mutex.unlock();
				continue;
			}			
			
			if(!abort) {
			for (float radius = firstRing; radius <= lastRing; radius++) {
			
				// Get the cartesian points
				xCenter = gridData->getCartesianRefPointI();
				yCenter = gridData->getCartesianRefPointJ();

				// Get the data
				int numData = gridData->getCylindricalAzimuthLength(radius, height);
				float* ringData = new float[numData];
				float* ringAzimuths = new float[numData];
				gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
				gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);
						
				// Call gbvtd
				if (vtd->analyzeRing(xCenter, yCenter, radius, height, numData, ringData,
									ringAzimuths, vtdCoeffs, vtdStdDev)) {
					if (vtdCoeffs[0].getParameter() == "VTC0") {
							// VT[v] = vtdCoeffs[0].getValue();
					} else {
						emit log(Message("Error retrieving VTC0 in vortex!"));
					} 
				} else {
					QString err = "No VTD winds at radius ";
					QString loc;
					err.append(loc.setNum(radius));
					err.append(", height ");
					err.append(loc.setNum(height));
					emit log(Message(err));
				}

				delete[] ringData;
				delete[] ringAzimuths;
				
				// All done with this radius and height, archive it
				archiveWinds(radius, height, maxCoeffs);
			}
			}
			mutex.unlock();
		}
		mutex.lock();
		
		// Clean up
		delete [] vtdCoeffs;
		delete vtd;
		
		// Integrate the winds to get the pressure deficit at the 2nd level (presumably 2km)
		pressureDeficit = new float[(int)lastRing+1];
		getPressureDeficit(firstLevel+1);
		
		// Get the central pressure
		calcCentralPressure();
		vortexData->setPressure(centralPressure);
		vortexData->setPressureUncertainty(centralPressureStdDev);
		vortexData->setPressureDeficit(pressureDeficit[(int)lastRing]-pressureDeficit[(int)firstRing]);
		
		if(!foundWinds)
		{
			// Some error occurred, notify the user
			emit log(Message("Vortex Error!"));
			return;
		} else {
			// Update the vortex list
			// vortexData ???
		
			// Update the progress bar and log
			emit log(Message("Done with Vortex",90));

			// Let the AnalysisThread know we're done
			emit(windsFound());
		}
		mutex.unlock();
		
		// Go to sleep, wait for more data
		mutex.lock();
		if (!abort)
		{
			// Wait until new data is available
			waitForData.wait(&mutex);
			mutex.unlock();		
		}
		if(abort)
		  return;

		emit log(Message("End of Vortex Run"));
	}
}

void VortexThread::archiveWinds(float& radius, float& height, float& maxCoeffs)
{

	// Save the centers to the VortexData object
	int level = int(height - firstLevel);
	int ring = int(radius - firstRing);
	
	for (int coeff = 0; coeff < (int)(maxCoeffs); coeff++) {
		vortexData->setCoefficient(level, ring, coeff, vtdCoeffs[coeff]);
	}
	
}

void VortexThread::getPressureDeficit(const float& height)
{

	float* dpdr = new float[(int)lastRing+1];

	// Assuming radius is in KM here, when we correct for units later change this
	float deltar = 1000;
	
	// Initialize arrays
	for (float radius = 0; radius <= lastRing; radius++) {
		dpdr[(int)radius] = -999;
		pressureDeficit[(int)radius] = 0;
	}
	
	// Get coriolis parameter
	float f = 2 * 7.29e-5 * sin(vortexData->getLat(int(height-firstLevel)));
	
	for (float radius = firstRing; radius <= lastRing; radius++) {
		if (vortexData->getCoefficient((int)height, (int)radius, 0).getParameter() == "VTC0") {
			float meanVT = vortexData->getCoefficient((int)height, (int)radius, 0).getValue();
			if (meanVT != 0) {
				dpdr[(int)radius] = ((f * meanVT) + (meanVT * meanVT)/(radius * deltar)) * rhoBar[(int)height-1];
			}
		}
	}
	if (dpdr[(int)lastRing] != -999) {
		pressureDeficit[(int)lastRing] = -dpdr[(int)lastRing] * deltar * 0.001;
	}
	
	for (float radius = lastRing-1; radius >= 0; radius--) {
		if (radius >= firstRing) {
			if ((dpdr[(int)radius] != -999) and (dpdr[(int)radius+1] != -999)) {
				pressureDeficit[(int)radius] = pressureDeficit[(int)radius+1] - 
					(dpdr[(int)radius] + dpdr[(int)radius+1]) * deltar * 0.001/2;
			} else if (dpdr[(int)radius] != -999) {
				pressureDeficit[(int)radius] = pressureDeficit[(int)radius+1] - 
					dpdr[(int)radius] * deltar * 0.001;
			} else if (dpdr[(int)radius+1] != -999) {
				pressureDeficit[(int)radius] = pressureDeficit[(int)radius+1] - 
					dpdr[(int)radius+1] * deltar * 0.001;
			}
		} else {
			pressureDeficit[(int)radius] = pressureDeficit[(int)firstRing];
		}
	}
	
	delete [] dpdr;
	
}

void VortexThread::calcCentralPressure()
{
  /*
  // Set the maximum allowable radius and time difference
  // Need to make this user configurable
  float maxObRadius = lastRing + 50;
  float maxObTimeDiff = 59 * 60;
  */
  QDomElement pressure = configData->getConfig("pressure");
  float maxObTimeDiff = configData->getParam(pressure, "maxobstime").toFloat();
  float maxObRadius = 0;
  if(configData->getParam(pressure, "maxobsmethod") == "center")
    maxObRadius = configData->getParam(pressure, "maxobdist").toFloat();
  if(configData->getParam(pressure, "maxobsmethod") == "ring")
    maxObRadius = lastRing+configData->getParam(pressure, "maxobsdist").toFloat();

	// Sum values to hold pressure estimates
	float pressWeight = 0;
	float pressSum = 0;
	int numEstimates = 0;
	float pressEstimates[100];
	float weightEstimates[100];
	
	// Iterate through the pressure data
	for (int i = 0; i < pressureList->size(); i++) {
		float obPressure = pressureList->at(i).getPressure();
		if (obPressure > 0) {
			// Check the time
			QDateTime time = pressureList->at(i).getTime();
			int obTimeDiff = time.secsTo(vortexData->getTime());
			if ((obTimeDiff > 0) and (obTimeDiff <= maxObTimeDiff)) {
				// Check the distance
				float vortexLat = vortexData->getLat(1);
				float vortexLon = vortexData->getLon(1);
				float obLat = pressureList->at(i).getLat();
				float obLon = pressureList->at(i).getLon();
				float* relDist = gridData->getCartesianPoint(&vortexLat, &vortexLon,
															 &obLat, &obLon);
				float obRadius = sqrt(relDist[0]*relDist[0] + relDist[1]*relDist[1]);
				delete [] relDist;
			
				if ((obRadius >= vortexData->getRMW(1)) and (obRadius <= maxObRadius)) {
					// Good ob anchor!
					float pPrimeOuter;
					if (obRadius >= lastRing) {
						pPrimeOuter = pressureDeficit[(int)lastRing];
					} else {
						pPrimeOuter = pressureDeficit[(int)obRadius];
					}
					float cpEstimate = obPressure - (pPrimeOuter - pressureDeficit[0]);
					float weight = (((maxObTimeDiff - obTimeDiff) / maxObTimeDiff) +
									((maxObRadius - obRadius) / maxObRadius)) / 2;
					
					// Sum the estimate and save the value for Std Dev calculation
					pressWeight += weight;
					pressSum += (weight * cpEstimate);
					pressEstimates[numEstimates] = cpEstimate;
					weightEstimates[numEstimates] = weight;
					numEstimates++;
					
					// Log the estimate
					QString station = pressureList->at(i).getStationName();
					QString pressLog = "Anchor pressure " + QString::number(obPressure) + " found at " + station
						+ " " + time.toString(Qt::ISODate) + "," + QString::number(obRadius) + " km"
						+ "(CP = " + QString::number(cpEstimate) + ")";
					emit log(Message(pressLog));
				} else if (obRadius < vortexData->getRMW(1)) {
					// Close enough to be called a central pressure
					// Let PollThread handle that one
				}
			}
		}
	}
	
	// Should have a sum of pressure estimates now, if not use 1013
	float avgPressure = 0;
	float avgWeight = 0;
	if (numEstimates > 0) {
		avgPressure = pressSum/pressWeight;
		avgWeight = pressWeight/(float)numEstimates;
	
		// Calculate the standard deviation of the estimates and set the global variables
		float sumSquares = 0;
		for (int i = 0; i < numEstimates; i++) {
			float square = weightEstimates[i] * (pressEstimates[i] - avgPressure) * (pressEstimates[i] - avgPressure);
			sumSquares += square;
		}
		
		centralPressureStdDev = sumSquares/(avgWeight * (numEstimates-1));
		centralPressure = avgPressure;
		
	
	} else {
		// Assume standard environmental pressure
		centralPressure = 1013 - (pressureDeficit[(int)lastRing] - pressureDeficit[0]);
		centralPressureStdDev = 5;
	}
	
}

void VortexThread::catchLog(const Message& message)
{
  emit log(message);
}


