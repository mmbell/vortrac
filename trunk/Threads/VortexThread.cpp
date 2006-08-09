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

	  abort = false;
}

VortexThread::~VortexThread()
{
  mutex.lock();
  abort = true;
  waitForData.wakeOne();
  mutex.unlock();
  
  // Wait for the thread to finish running if it is still processing
  wait();

}

void VortexThread::getWinds(Configuration *wholeConfig, GriddedData *dataPtr,
							   float* vortexLat, float* vortexLon, VortexList* vortexPtr)
{

	// Lock the thread
	QMutexLocker locker(&mutex);

	// Set the grid object
	gridData = dataPtr;

	// Remember the vortex center
	refLat = vortexLat;
	refLon = vortexLon;
	
	// Set the vortex data object
	vortexResults = vortexPtr;
	
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
		
		// Create a GBVTD object to process the rings
		vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
		vtdCoeffs = new Coefficient[20];

		// Create a vortexData object to hold the results;
		vortexData = new VortexData(int(lastLevel - firstLevel + 1), int(lastRing - firstRing + 1), (int)maxWave);
		
		mutex.unlock();
		// Loop through the levels and rings
		for (float height = firstLevel; height <= lastLevel; height++) {
		        mutex.lock();
			if(!abort) {
			for (float radius = firstRing; radius <= lastRing; radius++) {
				// Set the reference point
				gridData->setAbsoluteReferencePoint(*refLat, *refLon, height);
				float RefI = gridData->getRefPointI();
				float RefJ = gridData->getRefPointJ();
				float RefK = gridData->getRefPointK();
				
				// Get the data
				gridData->setCartesianReferencePoint(int(RefI),int(RefJ),int(RefK));
				int numData = gridData->getCylindricalAzimuthLength(radius, height);
				float* ringData = new float[numData];
				float* ringAzimuths = new float[numData];
				gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
				gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);
						
				// Call gbvtd
				if (vtd->analyzeRing(RefI, RefJ, radius, height, numData, ringData,
									ringAzimuths, vtdCoeffs, vtdStdDev)) {
					if (vtdCoeffs[0].getParameter() == "VTC0") {
							// VT[v] = vtdCoeffs[0].getValue();
					} else {
						emit log(Message("Error retrieving VTC0 in vortex!"));
					} 
				} else {
					emit log(Message("VTD failed!"));
				}

				delete[] ringData;
				delete[] ringAzimuths;
				
				float numCoeffs = 0;
				// All done with this radius and height, archive it
				archiveWinds(radius, height, numCoeffs);
			}
			}
			mutex.unlock();
		}
		mutex.lock();

		// Vortex run complete! Save the results to a file
		vortexResults->append(*vortexData);
		vortexResults->save();
		
		// Clean up
		delete vtdCoeffs;
		delete vtd;
		
		if(!foundWinds)
		{
			// Some error occurred, notify the user
			emit log(Message("Vortex Error!"));
			return;
		} else {
			// Update the vortex list
			// vortexData ???
		
			// Update the progress bar and log
			emit log(Message("Done with Vortex",60));

			// Let the poller know we're done
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
		
		emit log(Message("End of Vortex Run"));
	}
}

void VortexThread::archiveWinds(float& radius, float& height, float& numCoeffs)
{

	// Save the centers to the VortexData object
	int level = int(height - firstLevel);
	int ring = int(radius - firstRing);
	for (int coeff = 0; coeff < (int)(numCoeffs); coeff++) {
		// Coefficient*  = new Center(Xind[point], Yind[point], VTind[point], level, ring);
		// vortexData->setCoefficient(level, ring, point, *indCenter);
	}
	
}

void VortexThread::catchLog(const Message& message)
{
  emit log(message);
}


