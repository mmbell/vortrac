/*
 *  SimplexThread.cpp
 *  vortrac
 *
 *  Created by Michael Bell on 2/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <QtGui>
#include <math.h>
#include "SimplexThread.h"
#include "DataObjects/Coefficient.h"
#include "DataObjects/Center.h"
#include "VTD/GBVTD.h"
#include "Math/Matrix.h"

SimplexThread::SimplexThread(QObject *parent)
  : QThread(parent)
{
  velNull = -999.;
  abort = false;
}

SimplexThread::~SimplexThread()
{
  mutex.lock();
  abort = true;
  waitForData.wakeOne();
  mutex.unlock();
  
  // Wait for the thread to finish running if it is still processing
  wait();

}

void SimplexThread::findCenter(Configuration *wholeConfig, GriddedData *dataPtr,
							   SimplexList* simplexPtr, VortexData* vortexPtr)
{

	// Lock the thread
	QMutexLocker locker(&mutex);

	// Set the grid object
	gridData = dataPtr;

	// Set the simplex list object
	simplexResults = simplexPtr;

	// Set the vortexdata object and initial center guess
	vortexData = vortexPtr;
	refLat = vortexData->getLat();
	refLon = vortexData->getLon();
	
	// Set the configuration info
	configData = wholeConfig;
	simplexConfig = wholeConfig->getConfig("center");
      
	// Start or wake the thread
	if(!isRunning()) {
		start();
	} else {
		waitForData.wakeOne();
	}
}

void SimplexThread::run()
{
  
	forever {
		// Check to see if we should quit
		if (abort)
		  return;

		// OK, Let's find a center
		mutex.lock();
		bool foundCenter = true;
		emit log(Message("Simplex search started"));
	
		// Initialize variables
		QString simplexPath = configData->getParam(simplexConfig, 
							   QString("dir"));
		QString geometry = configData->getParam(simplexConfig,
							QString("geometry"));
		QString velField = configData->getParam(simplexConfig,
							QString("velocity"));
		QString closure = configData->getParam(simplexConfig,
						       QString("closure"));
		
		firstLevel = configData->getParam(simplexConfig,
					     QString("bottomlevel")).toFloat();
		lastLevel = configData->getParam(simplexConfig,
					     QString("toplevel")).toFloat();
		firstRing = configData->getParam(simplexConfig,
					     QString("innerradius")).toFloat();
		lastRing = configData->getParam(simplexConfig, 
				       	     QString("outerradius")).toFloat();
		
		float boxSize = configData->getParam(simplexConfig, 
					     QString("boxdiameter")).toFloat();
		float numPoints = configData->getParam(simplexConfig, 
					     QString("numpoints")).toFloat();
		float boxRowLength = sqrt(numPoints);
		float boxIncr = boxSize / sqrt(numPoints);
		
		float radiusOfInfluence = configData->getParam(simplexConfig, 
					 QString("influenceradius")).toFloat();
		float convergeCriterion = configData->getParam(simplexConfig, 
					 QString("convergence")).toFloat();
		float maxIterations = configData->getParam(simplexConfig, 
					 QString("maxiterations")).toFloat();
		float ringWidth = configData->getParam(simplexConfig, 
					 QString("ringwidth")).toFloat();
		int maxWave = configData->getParam(simplexConfig, 
					 QString("maxwavenumber")).toInt();
		
		// Set the output directory??
		//simplexResults.setNewWorkingDirectory(simplexPath);
		
		// Define the maximum allowable data gaps
		dataGaps = new float[maxWave+1];
		for (int i = 0; i <= maxWave; i++) {
		  dataGaps[i] = configData->getParam(simplexConfig, 
			QString("maxdatagap"), QString("wavenum"), 
			QString().setNum(i)).toFloat();
		}
		
		// Create a GBVTD object to process the rings
		vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
		vtdCoeffs = new Coefficient[20];

		// Create a simplexData object to hold the results;
		simplexData = new SimplexData(int(lastLevel - firstLevel + 1), int(lastRing - firstRing + 1), (int)numPoints);
		simplexData->setTime(vortexData->getTime());
		simplexData->setNumPointsUsed((int)numPoints);
		
		// Allocate memory for the vertices	
		vertex = new float*[3];
		vertex[0] = new float[2];
		vertex[1] = new float[2];
		vertex[2] = new float[2];
		VT = new float[3];
		vertexSum = new float[2];
		mutex.unlock();
		// Loop through the levels and rings
		for (float height = firstLevel; height <= lastLevel; height++) {
			mutex.lock();
			
			// Set the reference point
			gridData->setAbsoluteReferencePoint(refLat, refLon, height);
			if ((gridData->getRefPointI() < 0) || 
				(gridData->getRefPointJ() < 0) ||
				(gridData->getRefPointK() < 0)) {
				// Out of bounds problem
				emit log(Message("Out of bounds in simplex!"));
				continue;
			}			

			if(!abort) {
			for (float radius = firstRing; radius <= lastRing; radius++) {
				
				// Set the corner of the box
				float cornerI = gridData->getCartesianRefPointI();
				float cornerJ = gridData->getCartesianRefPointJ();
				float cornerK = gridData->getCartesianRefPointK();
				float RefI = cornerI;
				float RefJ = cornerJ;
				float RefK = cornerK;
				if ((gridData->getRefPointI() < 0) || 
					(gridData->getRefPointJ() < 0)) {
					// Out of bounds problem
					emit log(Message("Out of bounds in simplex!"));
					continue;
				}
				// Initialize mean values
				int meanCount = 0;
				meanXall = meanYall= meanVTall = 0;
				meanX = meanY = meanVT = 0;
				stdDevVertexAll = stdDevVTAll = 0;
				stdDevVertex = stdDevVT = 0;
				convergingCenters = 0;
				
				// Loop through the initial guesses
				for (int point = 0; point <= numPoints-1; point++) {
					if (point <= boxRowLength) {
						RefI = cornerI + float(point) * boxIncr;
					} else { 
						RefI = cornerI + float((point)%int(boxRowLength)) * boxIncr;
					}
					
					RefJ = cornerJ + float(point/int(boxRowLength)) * boxIncr;
					
					// Initialize vertices
					float sqr32 = 0.866025;
					
					vertex[0][0] = RefI;
					vertex[0][1] = RefJ + radiusOfInfluence;
					vertex[1][0] = RefI + sqr32*radiusOfInfluence;
					vertex[1][1] = RefJ - 0.5*radiusOfInfluence;
					vertex[2][0] = RefI - sqr32*radiusOfInfluence;
					vertex[2][1] = RefJ - 0.5*radiusOfInfluence;
					vertexSum[0] = 0;
					vertexSum[1] = 0;
					
					for (int v=0; v <= 2; v++) {				
						// Get the data
						gridData->setCartesianReferencePoint(int(vertex[v][0]),int(vertex[v][1]),int(RefK));
						int numData = gridData->getCylindricalAzimuthLength(radius, height);
						float* ringData = new float[numData];
						float* ringAzimuths = new float[numData];
						gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
						gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);
						
						// Call gbvtd
						if (vtd->analyzeRing(vertex[v][0], vertex[v][1], radius, height, numData, ringData,
											 ringAzimuths, vtdCoeffs, vtdStdDev)) {
							if (vtdCoeffs[0].getParameter() == "VTC0") {
								VT[v] = vtdCoeffs[0].getValue();
							} else {
								emit log(Message("Error retrieving VTC0 in simplex!"));
							} 
						} else {
							VT[v] = -999;
							// emit log(Message("VTD failed!"));
						}

						delete[] ringData;
						delete[] ringAzimuths;
					}
					
					// Run the simplex search loop
					getVertexSum(vertex,vertexSum);
					int numIterations = 0;
					VTsolution = 0;
					Xsolution = 0;
					Ysolution = 0;
					int low = 0;
					int mid = 0;
					int high = 0;
						
					for(;;) {
						low = 0;
						// Sort the initial guesses
						high = VT[0] > VT[1] ? (mid = 1,0) : (mid = 0,1);
						for (int v=0; v<=2; v++) {
							if (VT[v] <= VT[low]) low = v;
							if (VT[v] > VT[high]) {
								mid = high;
								high = v;
							} else if (VT[v] > VT[mid] && v != high) mid = v;
						}
						
						// Check convergence
						float epsilon = 2.0 * fabs(VT[high]-VT[low])/(fabs(VT[high]) + fabs(VT[low]) + 1.0e-10);
						if (epsilon < convergeCriterion) {
							VTsolution = VT[high];
							Xsolution = vertex[high][0];
							Ysolution = vertex[high][1];
							break;
						}
						
						// Check iterations
						if (numIterations > maxIterations) {
							emit log(Message("Maximum iterations exceeded in Simplex!"));
							break;
						}
						
						numIterations += 2;
						// Reflection
						float VTtest = simplexTest(vertex, VT, vertexSum, radius, height, RefK, velField, low, -1.0);
						if (VTtest >= VT[high])
							// Better point than highest, so try expansion
							VTtest = simplexTest(vertex, VT, vertexSum, radius, height, RefK, velField, low, 2.0);
						else if (VTtest <= VT[mid]) { 
							// Worse point than second highest, so try contraction
							float VTsave = VT[low];
							VTtest = simplexTest(vertex, VT, vertexSum, radius, height, RefK, velField, low, 0.5);
							if (VTtest <= VTsave) {
								for (int v=0; v<=2; v++) {
									if (v != high) {
										for (int i=0; i<=1; i++) 
											vertex[v][i] = vertexSum[i] = 0.5*(vertex[v][i] + vertex[high][i]);
										gridData->setCartesianReferencePoint(int(vertex[v][0]),int(vertex[v][1]),int(RefK));
										int numData = gridData->getCylindricalAzimuthLength(radius, height);
										float* ringData = new float[numData];
										float* ringAzimuths = new float[numData];
										gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
										gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);
									
										// Call gbvtd
										if (vtd->analyzeRing(vertex[v][0], vertex[v][1], radius, height, numData, ringData,
															 ringAzimuths, vtdCoeffs, vtdStdDev)) {
											if (vtdCoeffs[0].getParameter() == "VTC0") {
												VT[v] = vtdCoeffs[0].getValue();
											} else {
												emit log(Message("Error retrieving VTC0 in simplex!"));
											} 
										} else {
											VT[v] = -999;
											// emit log(Message("Not enough data in VTD ring"));
										}
										
										delete[] ringData;
										delete[] ringAzimuths;

									}
								}
								numIterations += 2;
								getVertexSum(vertex,vertexSum);
							}
						} else --numIterations;
					}
					
					// Done with simplex loop, should have values for the current point
					if ((VTsolution < 100.) and (VTsolution > 0.)) {
						// Add to sum
						meanXall += Xsolution;
						meanYall += Ysolution;
						meanVTall += VTsolution;
						meanCount++;
						
						// Add to array for storage
						Xind[point] = Xsolution;
						Yind[point] = Ysolution;
						VTind[point] = VTsolution;
					} else {
						Xind[point] = -999;
						Yind[point] = -999;
						VTind[point] = -999;
					}
				}
				
				// All points done, calculate means
				meanXall = meanXall / float(meanCount);
				meanYall = meanYall / float(meanCount);
				meanVTall = meanVTall / float(meanCount);
				for (int i=0;i<numPoints;i++) {
					if ((Xind[i] != -999.) and (Yind[i] != -999.) and (VTind[i] != -999.)) {
						stdDevVertexAll += ((Xind[i] - meanXall)*(Xind[i] - meanXall)
										 + (Yind[i] - meanYall)*(Yind[i] - meanYall));
						stdDevVTAll += (VTind[i] - meanVTall)*(VTind[i] - meanVTall);
					}
				}
				stdDevVertexAll = sqrt(stdDevVertexAll/float(meanCount-1));
				stdDevVTAll = sqrt(stdDevVTAll/float(meanCount-1));
					
				// Now remove centers beyond 1 standard deviation
				meanCount = 0;
				for (int i=0;i<numPoints;i++) {
					if ((Xind[i] != -999.) and (Yind[i] != -999.) and (VTind[i] != -999.)) {
						float vertexDist = sqrt((Xind[i]-meanXall)*(Xind[i] - meanXall)
										  + (Yind[i] - meanYall)*(Yind[i] - meanYall));
						if (vertexDist < stdDevVertexAll) {
							Xconv[meanCount] = Xind[i];
							Yconv[meanCount] = Yind[i];
							VTconv[meanCount] = VTind[i];
							meanX += Xind[i];
							meanY += Yind[i];
							meanVT += VTind[i];
							meanCount++;
						}
					}
				}
				meanX = meanX / float(meanCount);
				meanY = meanY / float(meanCount);
				meanVT = meanVT / float(meanCount);
				convergingCenters = meanCount;
				for (int i=0;i<convergingCenters-1;i++) {
					stdDevVertex += ((Xconv[i] - meanX)*(Xconv[i] - meanX)
									 + (Yconv[i] - meanY)*(Yconv[i] - meanY));
					stdDevVT += (VTconv[i] - meanVT)*(VTconv[i] - meanVT);
				}
				stdDevVertex = sqrt(stdDevVertex/float(meanCount-1));
				stdDevVT = sqrt(stdDevVT/float(meanCount-1));
				
				// All done with this radius and height, archive it
				archiveCenters(radius, height, numPoints);
			}
			}
			mutex.unlock();
		}
		mutex.lock();

		// Simplex run complete! Save the results to a file
		simplexResults->append(*simplexData);
		simplexResults->save();
		
		//Now pick the best center
		centerFinder = new ChooseCenter(configData,*simplexResults,vortexData);
		foundCenter = centerFinder->findCenter();
	
		// Clean up
		delete[] dataGaps;
		delete[] vertex;
		delete[] VT;
		delete[] vertexSum;
		delete[] vtdCoeffs;
		delete   vtd;
		
		if(!foundCenter)
		{
			// Some error occurred, notify the user
			emit log(Message("Simplex Error!"));
			return;
		} else {
			// Update the vortex list
			// vortexData ???
		
			// Update the progress bar and log
			emit log(Message("Done with Simplex",60));

			// Let the poller know we're done
			emit(centerFound());
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
		
	}
}

void SimplexThread::archiveCenters(float& radius, float& height, float& numPoints)
{

	// Save the centers to the SimplexData object
	int level = int(height - firstLevel);
	int ring = int(radius - firstRing);
	simplexData->setHeight(level, height);
	simplexData->setRadius(ring, radius);
	simplexData->setX(level, ring, meanX);
	simplexData->setY(level, ring, meanY);
	simplexData->setMaxVT(level, ring, meanVT);
	simplexData->setCenterStdDev(level, ring, stdDevVertex);
	simplexData->setVTUncertainty(level, ring, stdDevVT);
	simplexData->setNumConvergingCenters(level, ring, (int)convergingCenters);
	for (int point = 0; point < (int)numPoints; point++) {
	  Center indCenter(Xind[point], Yind[point], VTind[point], level, ring);
	  simplexData->setCenter(level, ring, point, indCenter);
	}
	
}

void SimplexThread::catchLog(const Message& message)
{
  emit log(message);
}

inline void SimplexThread::getVertexSum(float**& vertex,float*& vertexSum)
{
	
	float sum;
	int v;
	for (int i=0; i<=1; i++) {
		for (sum = 0.0, v=0; v<=2; v++)
			sum += vertex[v][i];
		vertexSum[i] = sum;
	}
}

float SimplexThread::simplexTest(float**& vertex,float*& VT,float*& vertexSum, 
								 float& radius, float& height, float& RefK,
								 QString& velField, int& low, double factor)
{
	
	// Test a simplex vertex
	float VTtest;
	float* vertexTest = new float[2];
	float factor1 = (1.0 - factor)/2;
	float factor2 = factor1 - factor;
	for (int i=0; i<=1; i++)
		vertexTest[i] = vertexSum[i]*factor1 - vertex[low][i]*factor2;
	
	// Get the data
	gridData->setCartesianReferencePoint(int(vertexTest[0]),int(vertexTest[1]),int(RefK));
	int numData = gridData->getCylindricalAzimuthLength(radius, height);
	float* ringData = new float[numData];
	float* ringAzimuths = new float[numData];
	gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
	gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);
	
	// Call gbvtd
	if (vtd->analyzeRing(vertexTest[0], vertexTest[1], radius, height, numData, ringData,
						 ringAzimuths, vtdCoeffs, vtdStdDev)) {
		if (vtdCoeffs[0].getParameter() == "VTC0") {
			VTtest = vtdCoeffs[0].getValue();
		} else {
			emit log(Message("Error retrieving VTC0 in simplex!"));
		} 
	} else {
		VTtest = -999;
		// emit log(Message("Not enough data in simplex ring"));
	}

	delete[] ringData;
	delete[] ringAzimuths;

	// If its a better point than the worst, replace it
	if (VTtest > VT[low]) {
		VT[low] = VTtest;
		for (int i=0; i<=1; i++) {
			vertexSum[i] += vertexTest[i]-vertex[low][i];
			vertex[low][i] = vertexTest[i];
		}
	}
	delete[] vertexTest;
	return VTtest;

}


