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
#include <cmath>
#include "SimplexThread.h"
#include "DataObjects/Coefficient.h"
#include "VTD/GBVTD.h"

SimplexThread::SimplexThread(QObject *parent)
  : QThread(parent)
{
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

void SimplexThread::findCenter(QDomElement centerConfig, GriddedData *dataPtr, float* vortexLat, float* vortexLon)
{

	// Lock the thread
	QMutexLocker locker(&mutex);

	// Set the grid object
	gridData = dataPtr;

	// Remember the vortex center
	refLat = vortexLat;
	refLon = vortexLon;

	// Set the configuration info
	simplexConfig = centerConfig;

	// Start or wake the thread
	if(!isRunning()) {
		start();
	} else {
		waitForData.wakeOne();
	}
}

void SimplexThread::run()
{
	emit log(Message("SimplexThread Started"));
  
	forever {
		// Check to see if we should quit
		if (abort)
		  return;

		// OK, Let's find a center
		mutex.lock();
		bool foundCenter = true;
		
		// Initialize variables
		QString simplexPath = simplexConfig.firstChildElement("dir").text();		
		QString geometry = simplexConfig.firstChildElement("geometry").text();
		QString velField = simplexConfig.firstChildElement("velocity").text();
		QString closure = simplexConfig.firstChildElement("closure").text();
		
		float firstLevel = simplexConfig.firstChildElement("bottomlevel").text().toFloat();
		float lastLevel = simplexConfig.firstChildElement("toplevel").text().toFloat();
		float firstRing = simplexConfig.firstChildElement("innerradius").text().toFloat();
		float lastRing = simplexConfig.firstChildElement("outerradius").text().toFloat();
		
		float boxSize = simplexConfig.firstChildElement("boxdiameter").text().toFloat();
		float numPoints = simplexConfig.firstChildElement("numpoints").text().toFloat();
		float boxRowLength = sqrt(numPoints);
		float boxIncr = boxSize / sqrt(numPoints);
		
		float radiusOfInfluence = simplexConfig.firstChildElement("influenceradius").text().toFloat();
		float convergeCriterion = simplexConfig.firstChildElement("convergence").text().toFloat();
		float maxIterations = simplexConfig.firstChildElement("maxIterations").text().toFloat();
		float ringWidth = simplexConfig.firstChildElement("ringwidth").text().toFloat();
		int maxWave = simplexConfig.firstChildElement("maxwavenumber").text().toInt();
		
		dataGaps = new float[maxWave];
		for (int i = 0; i <= maxWave; i++) {
			QString maxGap = "maxdatagap_" + i;
			dataGaps[i] = simplexConfig.firstChildElement(maxGap).text().toFloat();
		}
		
		// Create a GBVTD object to process the rings
		vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
		vtdCoeffs = new Coefficient[20];

		for (float height = firstLevel; height <= lastLevel; height++) {
			for (float radius = firstRing; radius <= lastRing; radius++) {
				// Set the reference point
				gridData->setAbsoluteReferencePoint(*refLat, *refLon, height);
				float RefI = gridData->getRefPointI();
				float RefJ = gridData->getRefPointJ();
				float RefK = gridData->getRefPointK();
				
				// Initialize mean values
				int meanCount = 0;
				meanXall = meanYall= meanVTall = 0;
				meanX = meanY = meanVT = 0;
				stdDevVertexAll = stdDevVTAll = 0;
				stdDevVertex = stdDevVT = 0;
				convergingCenters = 0;
				
				for (int point = 0; point <= numPoints-1; point++) {
					if (point <= boxRowLength) {
						RefI = RefI + float(point) * boxIncr;
					} else { 
						RefI = RefI + float((point)%int(boxRowLength)) * boxIncr;
					}
					
					RefJ = RefJ + float(point/int(boxRowLength)) * boxIncr;

					// Initialize vertices
					float** vertex = new float*[2];
					float* VT = new float[3];
					float* vertexSum = new float[2];
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
						float* ringData = gridData->getCylindricalAzimuthData(velField, radius, height);
						float* ringAzimuths = gridData->getCylindricalAzimuthPosition(radius, height);
						float thetaT = atan2(vertex[v][1],vertex[v][0]);
						
						// Call gbvtd
						if (vtd->analyzeRing(vertex[v][0], vertex[v][1], radius, numData, ringData,
											 ringAzimuths, vtdCoeffs, vtdStdDev)) {
							if (vtdCoeffs[0].getParameter() == "VTC0") {
								VT[v] = -(vtdCoeffs[0].getValue());
							} else {
								emit log(Message("Error retrieving VTC0 in simplex!"));
							} 
						} else {
							emit log(Message("Error retrieving VTC0 in simplex!"));
						}

					}
					
					// Run the simplex search loop
					getVertexSum(vertex,vertexSum);
					for(;;) {
						int numIterations = 0;
						int low = 0;
						int mid = 0;
						int high = 0;
						VTsolution = 0;
						Xsolution = 0;
						Ysolution = 0;
						
						// Sort the initial guesses
						high = VT[0] > VT[1] ? (mid = 1,0) : (mid = 0,1);
						for (int v=0; v<=1; v++) {
							if (VT[v] <= VT[low]) low = v;
							if (VT[v] > VT[high]) {
								mid = high;
								high = v;
							} else if (VT[v] > VT[mid] && v != high) mid = v;
						}
						
						// Check convergence
						float epsilon = 2.0 * fabs(VT[high]-VT[low])/(fabs(VT[high]) + fabs(VT[low]) + 1.0e-10);
						if (epsilon < convergeCriterion) {
							VTsolution = -VT[low];
							Xsolution = vertex[low][0];
							Ysolution = vertex[low][1];
							break;
						}
						
						// Check iterations
						if (numIterations > maxIterations) {
							emit log(Message("Maximum iterations exceeded in Simplex!"));
						}
						
						numIterations += 2;
						// Reflection
						float VTtest = simplexTest(vertex, VT, vertexSum, radius, height, RefK, velField, high, -1.0);
						if (VTtest <= VT[low])
							// Better point, so try expansion
							VTtest = simplexTest(vertex, VT, vertexSum, radius, height, RefK, velField, high, 2.0);
						else if (VTtest >= VT[mid]) { 
							// Worse point, so try contraction
							float VTsave = VT[high];
							VTtest = simplexTest(vertex, VT, vertexSum, radius, height, RefK, velField, high, 0.5);
							if (VTtest >= VTsave) {
								for (int v=0; v<=2; v++) {
									if (v != low) {
										for (int i=0; i<=1; i++) 
											vertex[v][i] = vertexSum[i] = 0.5*(vertex[v][i] + vertex[low][i]);
											gridData->setCartesianReferencePoint(int(vertex[v][0]),int(vertex[v][1]),int(RefK));
											int numData = gridData->getCylindricalAzimuthLength(radius, height);
											float* ringData = gridData->getCylindricalAzimuthData(velField, radius, height);
											float* ringAzimuths = gridData->getCylindricalAzimuthPosition(radius, height);
										
										// Call gbvtd
										//gbvtd(numData, ringData, ringAzimuths, vtdCoeffs, stdDev);
										if (vtdCoeffs[0].getParameter() == "VTC0") {
											VT[v] = -(vtdCoeffs[0].getValue());
										} else {
											emit log(Message("Error retrieving VTC0 in simplex!"));
										}
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
				for (int i=0;i<=numPoints;i++) {
					if ((Xind[i] != -999.) and (Yind[i] != -999.) and (VTind[i] != -999.)) {
						stdDevVertexAll += ((Xind[i] - meanXall)*(Xind[i] - meanXall)
										 + (Yind[i] - meanYall)*(Yind[i] - meanYall));
						stdDevVTAll += (VTind[i] - meanVTall)*(VTind[i] - meanVTall);
					}
					stdDevVertexAll = sqrt(stdDevVertexAll/float(meanCount));
					stdDevVTAll = sqrt(stdDevVTAll/float(meanCount));
				}
				// Now remove centers beyond 1 standard deviation
				meanCount = 0;
				for (int i=0;i<=numPoints;i++) {
					if ((Xind[i] != -999.) and (Yind[i] != -999.) and (VTind[i] != -999.)) {
						float vertexDist = sqrt((Xind[i]-meanXall)*(Xind[i] - meanXall)
										  + (Yind[i] - meanYall)*(Yind[i] - meanYall));
						if (vertexDist < stdDevVertexAll) {
							Xconv[meanCount] = Xind[i];
							Yconv[meanCount] = VTind[i];
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
				for (int i=0;i<=numPoints;i++) {
					stdDevVertex += ((Xconv[i] - meanX)*(Xconv[i] - meanX)
									 + (Yconv[i] - meanY)*(Yconv[i] - meanY));
					stdDevVT += (VTconv[i] - meanVT)*(VTconv[i] - meanVT);
				}
				stdDevVertex = sqrt(stdDevVertex/float(meanCount));
				stdDevVT = sqrt(stdDevVT/float(meanCount));
				
				// All done with this radius and height, archive it
				archiveCenters(radius, height);
			}
		}

		// Simplex run complete! Now pick the best center
		//foundCenter = chooseCenter();
		
		if(!foundCenter)
		{
			// Some error occurred, notify the user
			emit log(Message("Simplex Error!"));
			return;
		} else {
			// Update the vortex list
			// vortexData ???
		
			// Update the progress bar and log
			emit log(Message("Found center!",60));

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
		
		emit log(Message("End of Simplex Run"));
	}
}

void SimplexThread::archiveCenters(float& radius, float& height)
{

	// Do something with the centers
	
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
								 QString& velField, int& high, double factor)
{
	
	// Test a simplex vertex
	float VTtest;
	float* vertexTest = new float[2];
	float factor1 = (1.0 - factor)/2;
	float factor2 = factor1 - factor;
	for (int i=0; i<=1; i++)
		vertexTest[i] = vertexSum[i]*factor1 - vertex[high][i]*factor2;
	
	// Get the data
	gridData->setCartesianReferencePoint(int(vertexTest[0]),int(vertexTest[1]),int(RefK));
	int numData = gridData->getCylindricalAzimuthLength(radius, height);
	float* ringData = gridData->getCylindricalAzimuthData(velField, radius, height);
	float* ringAzimuths = gridData->getCylindricalAzimuthPosition(radius, height);
	
	// Call gbvtd
	if (vtd->analyzeRing(vertexTest[0], vertexTest[1], radius, numData, ringData,
						 ringAzimuths, vtdCoeffs, vtdStdDev)) {
		if (vtdCoeffs[0].getParameter() == "VTC0") {
			VTtest = -(vtdCoeffs[0].getValue());
		} else {
			emit log(Message("Error retrieving VTC0 in simplex!"));
		} 
	} else {
		emit log(Message("Error retrieving VTC0 in simplex!"));
	}
	// If its a better point than the worst, replace it
	if (VTtest < VT[high]) {
		VT[high] = VTtest;
		for (int i=0; i<=1; i++) {
			vertexSum[i] += vertexTest[i]-vertex[high][i];
			vertex[high][i] = vertexTest[i];
		}
	}
	
	return VTtest;

}

