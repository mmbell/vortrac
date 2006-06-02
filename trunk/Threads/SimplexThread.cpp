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

void SimplexThread::findCenter(Configuration *wholeConfig, GriddedData *dataPtr, float* vortexLat, float* vortexLon)
{

	// Lock the thread
	QMutexLocker locker(&mutex);

	// Set the grid object
	gridData = dataPtr;

	// Remember the vortex center
	refLat = vortexLat;
	refLon = vortexLon;

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
	emit log(Message("SimplexThread Started"));
  
	forever {
		// Check to see if we should quit
		if (abort)
		  return;

		// OK, Let's find a center
		mutex.lock();
		bool foundCenter = true;
		
		// Initialize variables
		QString simplexPath = configData->getParam(simplexConfig, 
							   QString("dir"));
		QString geometry = configData->getParam(simplexConfig,
							QString("geometry"));
		QString velField = configData->getParam(simplexConfig,
							QString("velocity"));
		QString closure = configData->getParam(simplexConfig,
						       QString("closure"));
		
		float firstLevel = configData->getParam(simplexConfig,
					     QString("bottomlevel")).toFloat();
		float lastLevel = configData->getParam(simplexConfig,
					     QString("toplevel")).toFloat();
		float firstRing = configData->getParam(simplexConfig,
					     QString("innerradius")).toFloat();
		float lastRing = configData->getParam(simplexConfig, 
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
					 QString("maxIterations")).toFloat();
		float ringWidth = configData->getParam(simplexConfig, 
					 QString("ringwidth")).toFloat();
		int maxWave = configData->getParam(simplexConfig, 
					 QString("maxwavenumber")).toInt();
		
		dataGaps = new float[maxWave];
		for (int i = 0; i <= maxWave; i++) {
		  dataGaps[i] = configData->getParam(simplexConfig, 
			QString("maxdatagap"), QString("wavenum"), 
			QString().setNum(i)).toFloat();
		}
		
		// Create a GBVTD object to process the rings
		vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
		vtdCoeffs = new Coefficient[20];

		vertex = new float*[3];
		vertex[0] = new float[2];
		vertex[1] = new float[2];
		vertex[2] = new float[2];
		VT = new float[3];
		vertexSum = new float[2];
		
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
						
						// Call gbvtd
						if (vtd->analyzeRing(vertex[v][0], vertex[v][1], radius, height, numData, ringData,
											 ringAzimuths, vtdCoeffs, vtdStdDev)) {
							if (vtdCoeffs[0].getParameter() == "VTC0") {
								VT[v] = vtdCoeffs[0].getValue();
							} else {
								emit log(Message("Error retrieving VTC0 in simplex!"));
							} 
						} else {
							emit log(Message("VTD failed!"));
						}

						delete[] ringData;
						delete[] ringAzimuths;
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
										float* ringData = gridData->getCylindricalAzimuthData(velField, radius, height);
										float* ringAzimuths = gridData->getCylindricalAzimuthPosition(radius, height);
									
										// Call gbvtd
										if (vtd->analyzeRing(vertex[v][0], vertex[v][1], radius, height, numData, ringData,
															 ringAzimuths, vtdCoeffs, vtdStdDev)) {
											if (vtdCoeffs[0].getParameter() == "VTC0") {
												VT[v] = vtdCoeffs[0].getValue();
											} else {
												emit log(Message("Error retrieving VTC0 in simplex!"));
											} 
										} else {
											emit log(Message("VTD failed!"));
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
	float* ringData = gridData->getCylindricalAzimuthData(velField, radius, height);
	float* ringAzimuths = gridData->getCylindricalAzimuthPosition(radius, height);
	
	// Call gbvtd
	if (vtd->analyzeRing(vertexTest[0], vertexTest[1], radius, height, numData, ringData,
						 ringAzimuths, vtdCoeffs, vtdStdDev)) {
		if (vtdCoeffs[0].getParameter() == "VTC0") {
			VTtest = vtdCoeffs[0].getValue();
		} else {
			emit log(Message("Error retrieving VTC0 in simplex!"));
		} 
	} else {
		emit log(Message("VTD failed!"));
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
	
	return VTtest;

}

bool SimplexThread::chooseCenter()
{
initialize();
//perlChooseCenter();

 return false;
}

bool SimplexThread::initialize()
{
  QDomElement ccElement = configData->getConfig("choosecenter");
  
  distWeight = configData->getParam(ccElement, QString("dist_weight")).toFloat();
  windWeight = configData->getParam(ccElement, QString("wind_weight")).toFloat();
  stdWeight = configData->getParam(ccElement, QString("stddev_weight")).toFloat();
  ptsWeight = configData->getParam(ccElement, QString("pts_weight")).toFloat();
  int fPercent = configData->getParam(ccElement, QString("stats")).toInt();
  QDate sDate = QDate::fromString(configData->getParam(ccElement, 
						   QString("startdate")),
				  Qt::ISODate);
  QDate eDate = QDate::fromString(configData->getParam(ccElement, 
						   QString("enddate")),
				  Qt::ISODate);
  QTime sTime = QTime::fromString(configData->getParam(ccElement, 
						   QString("starttime")),
				  Qt::ISODate);
  QTime eTime = QTime::fromString(configData->getParam(ccElement,
						   QString("endtime")),
				  Qt::ISODate);
  startTime = QDateTime(sDate);
  startTime.setTime(sTime);
  endTime = QDateTime(eDate);
  endTime.setTime(eTime);

  // Didn't initialize weightscheme array, see if we need this first

  if(fPercent = 99) {
    fCriteria[0] = 4052.2;  
    fCriteria[1] = 98.50;  
    fCriteria[2] = 34.12;  
    fCriteria[3] = 21.20;  
    fCriteria[4] = 16.26;  
    fCriteria[5] = 13.75;  
    fCriteria[6] = 12.25;  
    fCriteria[7] = 11.26;  
    fCriteria[8] = 10.56;  
    fCriteria[9] = 10.04;  
    fCriteria[10] = 9.65;  
    fCriteria[11] = 9.33;  
    fCriteria[12] = 9.07;  
    fCriteria[13] = 8.86;  
    fCriteria[14] = 8.68;  
    fCriteria[15] = 8.53;  
    fCriteria[16] = 8.40;  
    fCriteria[17] = 8.29;  
    fCriteria[18] = 8.18;  
    fCriteria[19] = 8.10;  
    fCriteria[20] = 8.02;  
    fCriteria[21] = 7.95;  
    fCriteria[22] = 7.88;  
    fCriteria[23] = 7.82;  
    fCriteria[24] = 7.77;  
    fCriteria[25] = 7.72;  
    fCriteria[26] = 7.68;  
    fCriteria[27] = 7.64;  
    fCriteria[28] = 7.60;  
    fCriteria[29] = 7.56;

  }
  else {
    fCriteria[0] = 161.45;  
    fCriteria[1] = 18.513;  
    fCriteria[2] = 10.128;  
    fCriteria[3] = 7.7086;  
    fCriteria[4] = 6.6079;  
    fCriteria[5] = 5.9874;  
    fCriteria[6] = 5.5914;  
    fCriteria[7] = 5.3177;  
    fCriteria[8] = 5.1174;  
    fCriteria[9] = 4.9646;  
    fCriteria[10] = 4.8443;  
    fCriteria[11] = 4.7472;  
    fCriteria[12] = 4.6672;  
    fCriteria[13] = 4.6001;  
    fCriteria[14] = 4.5431;  
    fCriteria[15] = 4.4940;  
    fCriteria[16] = 4.4513;  
    fCriteria[17] = 4.4139;  
    fCriteria[18] = 4.3808;  
    fCriteria[19] = 4.3513;  
    fCriteria[20] = 4.3248;  
    fCriteria[21] = 4.3009;  
    fCriteria[22] = 4.2793;  
    fCriteria[23] = 4.2597;  
    fCriteria[24] = 4.2417;  
    fCriteria[25] = 4.2252;  
    fCriteria[26] = 4.2100;  
    fCriteria[27] = 4.1960;  
    fCriteria[28] = 4.1830;  
    fCriteria[29] = 4.1709;
  }
  return false;
}
/*
bool SimplexThread::perlChooseCenter()
{
  bestRadius = new int*[simplexResults.count()];
  centerDev = new float[simplexResults.count()];
  radiusDev = new float[simplexResults.count()];
  score = new float**[simplexResults.count()];
  for(int i = 0; i < simplexResults.count(); i++) {
    bestRadius[i] = new int[simplexResults[i].getNumLevels()];
    score[i] = new float*[simplexResults[i].getNumRadii()];
    for(int rad = 0; rad < simplexResults[i].getNumRadii(); rad++) 
      score[i][rad] = new float[simplexResults[i].getNumLevels()];
    float radiusSum = 0;
    float xSum = 0; 
    float ySum = 0;
    for(int k = 0; k < simplexResults[i].getNumLevels(); k++) {
      int lastj = -1;
      float *winds = new float[simplexResults[i].getNumRadii()];
      float *stds = new float[simplexResults[i].getNumRadii()];
      float *pts = new float[simplexResults[i].getNumRadii()];
      float bestWind = 0.;
      float bestStd = 50.;
      float bestPts = 0.;
      float ptRatio = (float)simplexResults[i].getNumPointsUsed()/2.718281828;
      for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	winds[j] = velNull;
	stds[j] = velNull;
	pts[j] = velNull;
	if(simplexResults[i].getMaxVT(k,j)!=velNull) {
	  winds[j] = simplexResults[i].getMaxVT(k,j);
	  if(winds[j] > bestWind)
	    bestWind = winds[j];
	}
	if(simplexResults[i].getCenterStdDev(k,j)!=velNull) {
	  stds[j] = simplexResults[i].getCenterStdDev(k,j);
	  if(stds[j] < bestStd)
	    bestStd = stds[j];
	}
	if(simplexResults[i].getNumConvergingCenters(k,j)!=velNull) {
	  pts[j] = simplexResults[i].getNumConvergingCenters(k,j);
	  if(pts[j] > bestPts)
	    bestPts = pts[j];
	}
      }

      // Formerlly know as fix winds
      
      int count = 0;
      float *peakWinds = new float[simplexResults[i].getNumRadii()];
      float *peaks = new float[simplexResults[i].getNumRadii()];
      float meanPeak,meanStd;
      for(int a = 1; a < simplexResults[i].getNumRadii()-1; a++) {
	if((winds[a] >= winds[a-1])&&(winds[a] >=winds[a+1])) {
	  peakWinds[count] = winds[a];
	  count++;
	  peaks[a] = 1;
	}
	else {
	  peaks[a]=0;
    	}
      }
      float windSum = 0;
      for(int a = 0; a < count; a++) {
	windSum += peakWinds[a];
      }
      if(count>0) {
	meanPeak = windSum/((float)count);
	meanStd = 0;
	for(int z = 0; z < count; z++) 
	  meanStd += (peakWinds[z]-meanPeak)*(peakWinds[z]-meanPeak);
	meanStd/=count;
      }
      else {
	meanStd = 0;
	peakWinds = winds;
      }
      
      // End of function formally known as fix winds
      // returned meanpeak, meanstd, peaks[array]
      
      for(int jj = 0; jj < simplexResults[i].getNumRadii()-1; jj++) {
	if(((jj>0)&&(jj<simplexResults[i].getNumRadii()-1))
	   &&((peakWinds[jj]==1)||(peakWinds[jj+1]==1)||(peakWinds[jj-1]))) {
	  winds[jj] = simplexResults[i].getMaxVT(k,jj);
	}
	else {
	  winds[jj] = velNull;
	}
	
	if(winds[jj]!=velNull) {     // my version of sorting
	  if(winds[jj] > bestWind) {
	    bestWind = winds[jj];      
	  }
	}
	
	
	// bests from eachs were assigned earilier
	
	float tempBest = 0;
	// int bestIndex = 0;  wrong place to declare this ?
	for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	  score[i][j][k] = velNull;
	  float windScore = 0;
	  float stdScore = 0;
	  float ptsScore = 0;
	  //for(int w = 0; w <= #weightSchemes; w++) {
	  // Mike says we only need to run this once
	  // assign windWeight, stdWeight, ptsWeight, but not
	  // distWeight cause we are droping it
	  if((bestWind!=0)&&(winds[j]!=velNull))
	    windScore = exp(winds[j]-bestWind)*windWeight;
	  if(stds[j]!=velNull)
	    stdScore = bestStd/stds[j]*stdWeight;
	  if((bestPts!=0)&&(pts[j]!=velNull))
	    ptsScore = log((float)pts[j]/ptRatio)*ptsWeight;
	  
	  // How do we get log without log(const Message)??
	  
	  if(winds[j]!=velNull) {
	    // We don't want any score if the wind didn't hit near peak
	    score[i][j][k] = windScore+stdScore+ptsScore;
	    // I think we might want this to be totalscore too........
	    // ps score was never declared the original version had a w index
	    // totalScore was just suming them up over the w's
	    // but that was commented out???
	    
	    
	    // Use default weight scheme to set radius and mean 
	    if((score[i][j][k] > tempBest)&&(k>=0)
	       &&(k<=simplexResults[i].getNumLevels())) {
	      tempBest = score[i][j][k];
	      if(lastj == -1){
		// sort through to find the highest scoring x,y,radius
		// for each level, later we will sum those together over 
		// all levels and take the mean; if this is the first acceptable
		// radius found in a level we know because lastj is still set to 
		// -1, otherwise the value that was formerly the best must be
		// subtracted from the sums.
		lastj = j;
		radiusSum += simplexResults[i].getRadius(j);
		// gives the actual distance of the radius not just index
		xSum+= simplexResults[i].getX(j,k);
		ySum+= simplexResults[i].getY(j,k);
	      }
	      else {
		// if we have found a higher scoring radius on the same level
		// remove the old one and add the new
		// consider changing jLast to jHigh or something, jBest mayber
		radiusSum -= simplexResults[i].getRadius(j);
		// gives the actual distance of the radius not just index	     
		xSum-=simplexResults[i].getX(lastj,k);
		ySum-=simplexResults[i].getY(lastj,k);
		
		radiusSum += simplexResults[i].getRadius(j);
		xSum+=simplexResults[i].getX(j,k);
		ySum+=simplexResults[i].getY(j,k);
	      }
	    }
	  }
	}
      }
      // calculate the mean radius and center scores over all levels
      float numLevels = simplexResults[i].getNumLevels(); 
      // what is the current level index verses height issue
      float meanRadius = radiusSum/numLevels;
      float meanXChoose = xSum/numLevels;
      float meanYChoose = ySum/numLevels;
      
      //if(opt_i){
      //   calc_radscore();   we don't need this it is only related to crazy
      //}                     weight scheme stuff
      
      
      for(int k = 0; k <= simplexResults[i].getNumLevels(); k++) {
	int bestIndex;
	for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	  //score[i][j][k] /=(numWeightSchemes+1);
	  // totalscore[i][j][k] /=(numWeightSchemes+1);
	  // This was 2 in the code at least cause we added the max VTC0 default
	  // to whatever was in the file, I think this is one here
	}
	float bestScore = 0;
	for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	  //if(totalScore[i][j][k] > bestScore) {
	  //  bestScore = totalScore[i][j][k];
	  //  bestIndex = j;
	  //}
	  if(score[i][j][k] > bestScore) {
	    bestScore = score[i][j][k];
	    bestIndex = j;
	  }
	}
	bestRadius[i][k] = bestIndex;
      }
      // Calculate initial standard deviations of radius and center
      centerDev[i] = 0;
      radiusDev[i] = 0;
      
      for(int k = 0; k <= simplexResults[i].getNumLevels(); k++) {
	int j = bestRadius[i][k];
	float radLength = simplexResults[i].getRadius(j);
	radiusDev[i] +=(meanRadius-radLength)*(meanRadius-radLength);
	centerDev[i] +=sqrt(centerDev[i]/numLevels);
	// Do these get put in the simplexData???
      }
    }
  } 
}
*/
