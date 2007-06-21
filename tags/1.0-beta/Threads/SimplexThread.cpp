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
#include "HVVP/Hvvp.h"

SimplexThread::SimplexThread(QObject *parent)
  : QThread(parent)
{
  this->setObjectName("Simplex");
  velNull = -999.;
  abort = false;
  gridData = NULL;
  radarData = NULL;
  simplexData = NULL;
  simplexResults = NULL;
  vortexData = NULL;
  configData = NULL;
  
  dataGaps = NULL;
  vtd = NULL;
  vtdCoeffs = NULL;
  vertex = NULL;
  VT = NULL;
  vertexSum = NULL;
}

SimplexThread::~SimplexThread()
{
  //emit log(Message(QString("Entered SimplexThread Destructor")));
  
  waitForData.wakeOne(); // if we are not currently running the simplex thread this allows us
  // to wake it up and unlock and abort it.
  
  mutex.lock();  // Allows us to change abort waiting for control of member variables
  abort = true;  // Sets boolean member to stop work in thread at next opportunity
  emit centerFound();  // Allows analysisThread to stop waiting for the center to come back
  mutex.unlock(); // Allows the run loop to continue
  
  // Wait for the thread to finish running if it is still processing
  if(this->isRunning())
    wait(); 
  gridData = NULL; 
  delete gridData;
  radarData = NULL;
  delete radarData;
  simplexData = NULL;
  delete simplexData;
  simplexResults = NULL;
  delete simplexResults;
  vortexData = NULL;
  delete vortexData;
  configData = NULL;
  delete configData;
 
  //emit log(Message("Leaving the SimplexThread Destructor"));

}

void SimplexThread::findCenter(Configuration *wholeConfig,GriddedData *dataPtr,
			       RadarData* radarPtr, SimplexList* simplexPtr, 
			       VortexData* vortexPtr)
{

	// Lock the thread
  	QMutexLocker locker(&mutex);
        
	// Set the grid object
	gridData = dataPtr;

	// Set the radar object
	radarData = radarPtr;

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
    emit log(Message(QString("Simplex search started"),0,this->objectName()));
    
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
    QDomElement radar = configData->getConfig("radar");
    float radarLat = configData->getParam(radar, "lat").toFloat();
    float radarLon = configData->getParam(radar, "lon").toFloat();
    
    // Set the output directory??
    //simplexResults.setNewWorkingDirectory(simplexPath);
    
    // Define the maximum allowable data gaps
    dataGaps = new float[maxWave+1];
    for (int i = 0; i <= maxWave; i++) {
      dataGaps[i] = configData->getParam(simplexConfig, 
			       QString("maxdatagap"), QString("wavenum"), 
					 QString().setNum(i)).toFloat();
    }
    
    emit log(Message(QString(),2,this->objectName()));

    // Create a GBVTD object to process the rings

    if(closure.contains(QString("hvvp"),Qt::CaseInsensitive)) {
      float zero = 0;
      if(calcHVVP(zero,zero))
    	vtd = new GBVTD(geometry, closure, maxWave, dataGaps, hvvpResult);
      else {
    	emit log(Message(QString(),0,this->objectName(),Yellow,
    			 QString("HVVP Failure: Could Not Retrieve HVVP Winds")));
    	vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
    	}
    }
    else {
      vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
    }
    vtdCoeffs = new Coefficient[20];

    emit log(Message(QString(),3,this->objectName()));

    mutex.unlock();
    if(abort)
      return;
    mutex.lock();
    
    QDomElement cappi = configData->getConfig("cappi");
    float zgridsp = configData->getParam(cappi, "zgridsp").toFloat();
    
    int newNumLevels = (int)floor((lastLevel - firstLevel)/zgridsp +1.5);
    int newNumRings = (int)floor((lastRing - firstRing)/ringWidth + 1.5);

    // Create a simplexData object to hold the results;
    simplexData = new SimplexData(newNumLevels, newNumRings,(int)numPoints);
    
    // the number of levels should be divided by the zgridsp (cappi) because
    // those are measurements in km
    // the ring count should be divided by the ring width
      
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
    
    int numLevels = int(lastLevel-firstLevel+1);
    int loopPercent = int(20.0/float(numLevels));
    int endPercent = 20-(numLevels*loopPercent);
    
    // Loop through the levels and rings
    // Should this have some reference to grid spacing?
    for (float height = firstLevel; height <= lastLevel; height++) {
      emit log(Message(QString(), loopPercent, this->objectName()));

      mutex.lock();
      
      // Set the reference point
      gridData->setAbsoluteReferencePoint(refLat, refLon, height);

      mutex.unlock();
      
      if(!abort) {
	for (float radius = firstRing; radius <= lastRing; radius+=ringWidth) {
	  mutex.lock();
	  
	  // Set the corner of the box
	  float cornerI = gridData->getCartesianRefPointI();
	  float cornerJ = gridData->getCartesianRefPointJ();
	  float RefK = gridData->getCartesianRefPointK();
	  float RefI = cornerI;
	  float RefJ = cornerJ;
	  if ((gridData->getRefPointI() < 0) || 
	      (gridData->getRefPointJ() < 0) ||
	      (gridData->getRefPointK() < 0))  {
	    // Out of bounds problem
	    emit log(Message(QString("Initial simplex guess is outside CAPPI"),0,this->objectName()));
	    archiveNull(radius, height, numPoints);
	    mutex.unlock();
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

	  mutex.unlock();
	  for (int point = 0; point <= numPoints-1; point++) {
	    mutex.lock();
	    if (point < boxRowLength) {
	      RefI = cornerI + float(point) * boxIncr;
	    } else { 
	      RefI = cornerI + float((point)%int(boxRowLength)) * boxIncr;
	    }
	    
	    RefJ = cornerJ + float(point/int(boxRowLength)) * boxIncr;
	    
	    initialX[point] = RefI;
	    initialY[point] = RefJ;
	    
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
	      gridData->setCartesianReferencePoint(int(vertex[v][0]),
						   int(vertex[v][1]),
						   int(RefK));
	      if(closure.contains(QString("hvvp"),Qt::CaseInsensitive)) {
		float* hvvpLatLon = gridData->getAdjustedLatLon(radarLat,radarLon,int(vertex[v][0]),int(vertex[v][1]));
		
		if(calcHVVP(hvvpLatLon[0],hvvpLatLon[1]))
		  vtd->setHVVP(hvvpResult);
		else {
		  emit log(Message(QString("HVVP Failed to Locate Mean Wind"),0,this->objectName(),Yellow,QString("HVVP Failure")));
		  vtd->setHVVP(0);
		}
	      }
	      int numData = gridData->getCylindricalAzimuthLength(radius, height);
	      /*
		// For testing faster functions
	      int testNumData2 = gridData->getCylindricalAzimuthLengthTest2(radius, height);
	      if(numData != testNumData2)
		emit log(Message(QString(" Test2 Failed For radius "+QString().setNum(radius)+" and height "+QString().setNum(height)),0,this->objectName()));
	      */
	      float* ringData = new float[numData];
	      float* ringAzimuths = new float[numData];
	      gridData->getCylindricalAzimuthData(velField, numData, 
						  radius, height, ringData);
	      gridData->getCylindricalAzimuthPosition(numData, radius, 
						      height, ringAzimuths);
	      
	      // Call gbvtd
	      if (vtd->analyzeRing(vertex[v][0], vertex[v][1], radius, 
				   height, numData, ringData,
				   ringAzimuths, vtdCoeffs, vtdStdDev)) {
		if (vtdCoeffs[0].getParameter() == "VTC0") {
		  VT[v] = vtdCoeffs[0].getValue();
		} else {
		  emit log(Message(QString("Error retrieving VTC0 in simplex"),0,this->objectName()));
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

	    mutex.unlock();
	    
	    for(;;) {
	      if(abort)
		return;
	      
	      mutex.lock();
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
		mutex.unlock();
		break;
	      }
	      
	      // Check iterations
	      if (numIterations > maxIterations) {
		emit log(Message(QString("Maximum iterations exceeded in Simplex"),0,this->objectName()));
		mutex.unlock();
		break;
	      }
	      
	      numIterations += 2;
	      // Reflection
	      float VTtest = simplexTest(vertex, VT, vertexSum, radius, height,
					 RefK, velField, low, -1.0);
	      if (VTtest >= VT[high])
		// Better point than highest, so try expansion
		VTtest = simplexTest(vertex, VT, vertexSum, radius, height, 
				     RefK, velField, low, 2.0);
	      else if (VTtest <= VT[mid]) { 
		// Worse point than second highest, so try contraction
		float VTsave = VT[low];
		VTtest = simplexTest(vertex, VT, vertexSum, radius, height, 
				     RefK, velField, low, 0.5);
		if (VTtest <= VTsave) {
		  for (int v=0; v<=2; v++) {
		    if (v != high) {
		      for (int i=0; i<=1; i++)
			vertex[v][i] = vertexSum[i] = 0.5*(vertex[v][i] + vertex[high][i]);
		      gridData->setCartesianReferencePoint(int(vertex[v][0]),
							   int(vertex[v][1]),
							   int(RefK));
		      int numData = gridData->getCylindricalAzimuthLength(radius, height);
		      /*
			For testing faster functions
		      int testNumData2 = gridData->getCylindricalAzimuthLengthTest2(radius, height);
		      if(numData != testNumData2)
			emit log(Message(QString(" 2nd Test2 Failed For radius "+QString().setNum(radius)+" and height "+QString().setNum(height)),0,this->objectName()));
		      */
		      float* ringData = new float[numData];
		      float* ringAzimuths = new float[numData];
		      gridData->getCylindricalAzimuthData(velField, numData, 
							  radius, height, 
							  ringData);
		      gridData->getCylindricalAzimuthPosition(numData, radius, 
							      height, 
							      ringAzimuths);
		      
		      // Call gbvtd
		      
		      if (vtd->analyzeRing(vertex[v][0], vertex[v][1], radius, 
					   height, numData, ringData,
					   ringAzimuths, vtdCoeffs, vtdStdDev)) {
			if (vtdCoeffs[0].getParameter() == "VTC0") {
			  VT[v] = vtdCoeffs[0].getValue();
			} else {
			  emit log(Message(QString("Error retrieving VTC0 in simplex!"),0,this->objectName()));
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
	      mutex.unlock();
	    }

	    mutex.lock();
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
	    mutex.unlock();
	  }
	  mutex.lock();
	  // All points done, calculate means
	  if (meanCount == 0) {
	    // No answer!
	    archiveNull(radius, height, numPoints);
	  } else {
	    meanXall = meanXall / float(meanCount);
	    meanYall = meanYall / float(meanCount);
	    meanVTall = meanVTall / float(meanCount);
	    for (int i=0;i<numPoints;i++) {
	      if ((Xind[i] != -999.) and (Yind[i] != -999.) 
		  and (VTind[i] != -999.)) {
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
	    if (meanCount == 0) {
	      // No answer!
	      archiveNull(radius, height, numPoints);
	    } else {				
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
      }
      if(abort)
	return;
    }
    mutex.lock();
    
    //Message::toScreen("SimplexRun Complete");
    
    // Simplex run complete! Save the results to a file
    
    simplexResults->append(*simplexData);
    simplexResults->save();
    SimplexData *oldData = simplexData;
    simplexData = NULL;
    delete oldData;
    
    emit log(Message(QString(),1+endPercent,this->objectName()));
    
    //Now pick the best center
    simplexResults->timeSort();
    ChooseCenter *centerFinder = new ChooseCenter(configData,simplexResults,vortexData);
    connect(centerFinder, SIGNAL(errorlog(const Message&)),
	    this, SLOT(catchLog(const Message&)),Qt::DirectConnection);
    foundCenter = centerFinder->findCenter();
    
    // Save again to keep mean values found in chooseCenter
    simplexResults->save();
    
    emit log(Message(QString(),4,this->objectName()));

    // Clean up
    //delete centerFinder;
    delete[] dataGaps;
    for(int i = 2; i >=0; i--)
      delete [] vertex[i];
    delete[] vertex;
    delete[] VT;
    delete[] vertexSum;
    delete[] vtdCoeffs;
    delete   vtd;
    
    if(!foundCenter)
      {
	// Some error occurred, notify the user
	emit log(Message(QString("Failed to Indentify Center!"),0,this->objectName(),AllOff,QString(),SimplexError,QString("Simplex Failed")));
	return;
      } else {
	// Update the vortex list
	emit log(Message(QString("Done with Simplex"),0, this->objectName()));
	
	// Let the poller know we're done
	emit(centerFound());
      }
    mutex.unlock();
    
    // Go to sleep, wait for more data   
    if (!abort) {
      mutex.lock();
      // Wait until new data is available
      waitForData.wait(&mutex);	
      mutex.unlock();    
    }
    if(abort)
      return;
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
	  // We want to use the real radius and height in the center for use
	  // later so these should be given in km
	  // The level and ring integers are the storage positions
	  // these values are used for the indexing - LM 02/6/07
	  Center indCenter(Xind[point], Yind[point], VTind[point], 
			   height, radius);
	  simplexData->setCenter(level, ring, point, indCenter);
	  simplexData->setInitialX(level, ring, point, initialX[point]);
	  simplexData->setInitialY(level, ring, point, initialY[point]);
	}
	
}

void SimplexThread::archiveNull(float& radius, float& height, float& numPoints)
{
	
	// Save the centers to the SimplexData object
	int level = int(height - firstLevel);
	int ring = int(radius - firstRing);
	simplexData->setHeight(level, height);
	simplexData->setRadius(ring, radius);
	simplexData->setX(level, ring, -999);
	simplexData->setY(level, ring, -999);
	simplexData->setMaxVT(level, ring, -999);
	simplexData->setCenterStdDev(level, ring, -999);
	simplexData->setVTUncertainty(level, ring, -999);
	simplexData->setNumConvergingCenters(level, ring, (int)0);
	for (int point = 0; point < (int)numPoints; point++) {
		Center indCenter(-999, -999, -999, level, ring);
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
	/*
	  for testing faster functions
	int testNumData2 = gridData->getCylindricalAzimuthLengthTest2(radius, height);
	if(numData != testNumData2)
	  emit log(Message(QString(" 3rd Test2 Failed For radius "+QString().setNum(radius)+" and height "+QString().setNum(height)),0,this->objectName()));
	*/
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


bool SimplexThread::calcHVVP(float& lat, float& lon)
{
  // Get environmental wind
  /*
   * rt: Range from radar to circulation center (km).
   *
   * cca: Meteorological azimuth angle of the direction
   *      to the circulation center (degrees from north).
   *
   * rmw: radius of maximum wind measured from the TC circulation
   *      center outward.
   */

  QDomElement radar = configData->getConfig("radar");
  float radarLat = configData->getParam(radar,"lat").toFloat();
  float radarLon = configData->getParam(radar,"lon").toFloat();
  float vortexLat, vortexLon;
  if((lat == 0 )&&(lon == 0)) {
    QDomElement vortex = configData->getConfig("vortex");
    vortexLat = configData->getParam(vortex, "lat").toFloat();
    vortexLon = configData->getParam(vortex, "lon").toFloat();
  }
  else {
    vortexLat = lat;
    vortexLon = lon;
  }
  
  float* distance;
  distance = gridData->getCartesianPoint(&radarLat, &radarLon, 
					 &vortexLat, &vortexLon);
  float rt = sqrt(distance[0]*distance[0]+distance[1]*distance[1]);
  float cca = atan2(distance[0], distance[1])*180/acos(-1);
  delete[] distance;
  float rmw = 0;
  int goodrmw = 0;
  for(int level = 0; level < vortexData->getNumLevels(); level++) {
    if(vortexData->getRMW(level)!=-999) {
      //Message::toScreen("radius @ level "+QString().setNum(level)+" = "+QString().setNum(vortexData->getRMW(level)));
      rmw += vortexData->getRMW(level);
      goodrmw++;
    }
  }
  rmw = rmw/(1.0*goodrmw);
  // RMW is the average rmw taken over all levels of the vortexData
  // The multiplying by 2 bit is blatant cheating, I don't know 
  // if this if the radius's returned need to be adjusted by spacing or
  // or what the deal is but these numbers look closer to right for 
  // the two volumes I am looking at.

  //Message::toScreen("Hvvp Parameters: Distance to Radar "+QString().setNum(rt)+" angle to vortex center in degrees ccw from north "+QString().setNum(cca)+" rmw "+QString().setNum(rmw));
  //emit log(Message(QString("Hvvp Parameters: Distance to Radar "+QString().setNum(rt)+" angle to vortex center in degrees ccw from north "+QString().setNum(cca)+" rmw "+QString().setNum(rmw))));
  
  Hvvp *envWindFinder = new Hvvp;
  connect(envWindFinder, SIGNAL(log(const Message)), 
	  this, SLOT(catchLog(const Message)), 
	  Qt::DirectConnection);
  envWindFinder->setRadarData(radarData, rt, cca, rmw);
  //envWindFinder->findHVVPWinds(false); for first fit only
  bool hasHVVP = envWindFinder->findHVVPWinds(true);
  hvvpResult = envWindFinder->getAvAcrossBeamWinds();
  hvvpUncertainty = envWindFinder->getAvAcrossBeamWindsStdError();
  //  Message::toScreen("Hvvp gives "+QString().setNum(hvvpResult)+" +/- "+QString().setNum(hvvpUncertainty));
    
  return hasHVVP;
}

