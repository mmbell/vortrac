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
#include "HVVP/Hvvp.h"

VortexThread::VortexThread(QObject *parent)
  : QThread(parent)
{
    this->setObjectName("Vortex");

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
	  dataGaps = NULL;
}

VortexThread::~VortexThread()
{
  mutex.lock();
  abort = true;
  waitForData.wakeOne();
  mutex.unlock();

  gridData = NULL;
  radarVolume = NULL;
  vortexData = NULL;
  pressureList = NULL;
  configData = NULL;
  
  delete gridData;
  delete vortexData;
  delete pressureList;
  delete configData;

  delete [] dataGaps;
 
  // Wait for the thread to finish running if it is still processing
  wait();

}

void VortexThread::getWinds(Configuration *wholeConfig, GriddedData *dataPtr, RadarData *radarPtr, VortexData* vortexPtr, PressureList *pressurePtr)
{

	// Lock the thread
	QMutexLocker locker(&mutex);

	// Set the pressure List
	pressureList = pressurePtr;
	
	// Set the grid object
	gridData = dataPtr;

	// Set the radar object
	radarVolume = radarPtr;

	// Set the vortex data object
	vortexData = vortexPtr;
	
	// Set the configuration info
	configData = wholeConfig;
      
	// Start or wake the thread
	if(!isRunning()) {
		start();
	} else {
		waitForData.wakeOne();
	}
}

void VortexThread::run()
{
  //emit log(Message("VortexThread Started"));
  
	forever {
		// Check to see if we should quit
		if (abort)
		  return;

		// OK, Let's find a center
		mutex.lock();
		bool foundWinds = true;
		
		// Initialize variables
		readInConfig();
		
		// Set the output directory??
		//vortexResults.setNewWorkingDirectory(vortexPath);
	     
		float maxCoeffs = maxWave*2 + 3;

		// Find & Set Average RMW
		float rmw = 0;
		int goodrmw = 0;
		for(int l = 0; l < vortexData->getNumLevels(); l++) {
		  if((vortexData->getRMW(l)!=-999)&&(vortexData->getRMW(l)!=0)) {
		    //Message::toScreen("radius @ level "+QString().setNum(l)+" = "+QString().setNum(vortexData->getRMW(l)));
		    if(vortexData->getRMWUncertainty(l) < 10) {
		      rmw += vortexData->getRMW(l);
		      goodrmw++;
		    }
		  }
		}
		rmw = rmw/(1.0*goodrmw);
		vortexData->setAveRMW(rmw);
		// RMW is the average rmw taken over all levels of the vortexData
		
		// Create a GBVTD object to process the rings
		if(closure.contains(QString("hvvp"),Qt::CaseInsensitive)) {
		  if(calcHVVP(true)) {
		    vtd = new GBVTD(geometry, closure, maxWave, dataGaps, hvvpResult);
		    emit log(Message(QString(),6,this->objectName(),Green));
		  }
		  else {
		    emit log(Message(QString(),6,this->objectName(),Yellow,
				     QString("HVVP Failure: Could Not Retrieve HVVP Winds")));
		    vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
		  }
		}
		else {
		  vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
		  emit log(Message(QString(),6,this->objectName(),Green));
		}
		vtdCoeffs = new Coefficient[20];
	
		// Placeholders for centers
		float xCenter = -999;
		float yCenter = -999;
		
		mutex.unlock();
		// Loop through the levels and rings
		
		// We can't iterate through height like this and keep z spacing working -LM

		int loopPercent = int(7.0/float(gridData->getKdim()));
		int endPercent = 7-int(gridData->getKdim()*loopPercent);
		int storageIndex = -1;
		for(int h = 0; h < gridData->getKdim(); h++) {
		  emit log(Message(QString(),loopPercent,this->objectName()));
		  if(abort)
		    return;
		  float height = gridData->getCartesianPointFromIndexK(h);
		  if((height<firstLevel)||(height>lastLevel)) { continue; }
		  storageIndex++;
		  mutex.lock();
		  // Set the reference point
		  float referenceLat = vortexData->getLat(h);
		  float referenceLon = vortexData->getLon(h);
		  gridData->setAbsoluteReferencePoint(referenceLat, referenceLon, height);
		  if ((gridData->getRefPointI() < 0) || 
		      (gridData->getRefPointJ() < 0) ||
		      (gridData->getRefPointK() < 0)) {
		    // Out of bounds problem
		    emit log(Message(QString("Simplex center is outside CAPPI"),0,this->objectName()));
		    mutex.unlock();
		    continue;
		  }			
		  
		  if(!abort) {
		    // should we be incrementing radius using ringwidth? -LM
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
			  emit log(Message(QString("Error retrieving VTC0 in vortex!"),0,this->objectName()));
			} 
		      } else {
			QString err("Insufficient data for VTD winds: radius ");
			QString loc;
			err.append(loc.setNum(radius));
			err.append(", height ");
			err.append(loc.setNum(height));
			emit log(Message(err));
		      }

		      delete[] ringData;
		      delete[] ringAzimuths;
				
		      // All done with this radius and height, archive it
		      archiveWinds(radius, storageIndex, maxCoeffs);
		    }
		  }
		  mutex.unlock();
		}
		emit log(Message(QString(),endPercent,this->objectName()));

		mutex.lock();
		
		// Clean up
		delete [] vtdCoeffs;
		delete vtd;
		
		// Integrate the winds to get the pressure deficit at the 2nd level (presumably 2km)
		float* pressureDeficit = new float[(int)lastRing+1];
		getPressureDeficit(vortexData,pressureDeficit, gradientHeight);
		
		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();
	     		
		// Get the central pressure
		calcCentralPressure(vortexData,pressureDeficit, gradientHeight); // is firstLevel right?
		emit log(Message(QString(),3,this->objectName())); // 78 %

		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();

		// Get the central pressure uncertainty 
		/*
		 * For Gathering Stats
		 * calcPressureUncertainty(0, QString("CenterStdPresErr"));
		 * calcPressureUncertainty(1, QString("FlatPresErr"));
		 * calcPressureUncertainty(1.5, QString("FlatPresErr"));
		 */
		// 
		calcPressureUncertainty(1,QString());  
		//This is the one we are using?
		
		// 95 %
		
		delete [] pressureDeficit;
		float *temp = dataGaps;
		dataGaps = NULL;
		delete [] temp;
		
		if(!foundWinds)
		{
			// Some error occurred, notify the user
			emit log(Message(QString("Vortex Error!"),0,this->objectName()));
			return;
		} else {
			// Update the vortex list
			// vortexData ???
		
			// Update the progress bar and log
			emit log(Message(QString("Done with Vortex"),0,this->objectName()));

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

		emit log(Message(QString("End of Vortex Run"),0,
				 this->objectName()));
	}
}

void VortexThread::archiveWinds(float& radius, int& hIndex, float& maxCoeffs)
{

  // Save the centers to the VortexData object

  int level = hIndex;
  int ring = int(radius - firstRing);
	
  for (int coeff = 0; coeff < (int)(maxCoeffs); coeff++) {
    vortexData->setCoefficient(level, ring, coeff, vtdCoeffs[coeff]);
  }
	
}

void VortexThread::archiveWinds(VortexData& data, float& radius, int& hIndex, float& maxCoeffs)
{

  // Save the centers to the VortexData object

  int level = hIndex;
  int ring = int(radius - firstRing);
	
  for (int coeff = 0; coeff < (int)(maxCoeffs); coeff++) {
    data.setCoefficient(level, ring, coeff, vtdCoeffs[coeff]);
  }
	
}

void VortexThread::getPressureDeficit(VortexData* data, float* pDeficit,const float& height)
{

	float* dpdr = new float[(int)lastRing+1];

	// get the index of the height we want
	int heightIndex = data->getHeightIndex(height);
	//Message::toScreen("GetPressureDeficit for height "+QString().setNum(height)+" km which corresponds to index "+QString().setNum(heightIndex)+" in data; this corresponds to level "+QString().setNum(gridData->getIndexFromCartesianPointK(height)));

	// Assuming radius is in KM here, when we correct for units later change this
	float deltar = 1000;
	
	// Initialize arrays
	for (float radius = 0; radius <= lastRing; radius++) {
		dpdr[(int)radius] = -999;
		pDeficit[(int)radius] = 0;
	}
	
	// Get coriolis parameter
	float f = 2 * 7.29e-5 * sin(data->getLat(heightIndex)*3.141592653589793238462643/180.);
	
	for (float radius = firstRing; radius <= lastRing; radius++) {
		if (!(data->getCoefficient(height, radius, QString("VTC0")) == Coefficient())) {
			float meanVT = data->getCoefficient(height, radius, QString("VTC0")).getValue();
			if (meanVT != 0) {
				dpdr[(int)radius] = ((f * meanVT) + (meanVT * meanVT)/(radius * deltar)) * rhoBar[(int)height-1];
			}
		}
	}
	if (dpdr[(int)lastRing] != -999) {
		pDeficit[(int)lastRing] = -dpdr[(int)lastRing] * deltar * 0.001;
	}
	
	for (float radius = lastRing-1; radius >= 0; radius--) {
		if (radius >= firstRing) {
			if ((dpdr[(int)radius] != -999) and (dpdr[(int)radius+1] != -999)) {
				pDeficit[(int)radius] = pDeficit[(int)radius+1] - 
					(dpdr[(int)radius] + dpdr[(int)radius+1]) * deltar * 0.001/2;
			} else if (dpdr[(int)radius] != -999) {
				pDeficit[(int)radius] = pDeficit[(int)radius+1] - 
					dpdr[(int)radius] * deltar * 0.001;
			} else if (dpdr[(int)radius+1] != -999) {
				pDeficit[(int)radius] = pDeficit[(int)radius+1] - 
					dpdr[(int)radius+1] * deltar * 0.001;
			} else {
				// Flat extrapolation for data gaps > 2 radii
				pDeficit[(int)radius] = pDeficit[(int)radius+1];
			}
		} else {
			pDeficit[(int)radius] = pDeficit[(int)firstRing];
		}
	}
	delete [] dpdr;
	
}

void VortexThread::calcCentralPressure(VortexData* vortex, float* pD, float height)
{

  int heightIndex = vortex->getHeightIndex(height);
  float centralPressure = 0;

  // Sum values to hold pressure estimates
  emit log(Message(QString("Calculating Pressure at height = "+QString().setNum(height)+"km "),0,this->objectName()));
  float pressWeight = 0;
  float pressSum = 0;
  numEstimates = 0;
  float pressEstimates[100];
  float weightEstimates[100];
    
  // Iterate through the pressure data
  //Message::toScreen("Size of searching List = "+QString().setNum(pressureList->size())+" within time "+QString().setNum(maxObTimeDiff)+" of vortex time "+vortex->getTime().toString(Qt::ISODate));
  for (int i = 0; i < pressureList->size(); i++) {
    float obPressure = pressureList->at(i).getPressure();
    if (obPressure > 0) {
      // Check the time
      QDateTime time = pressureList->at(i).getTime();
      if(time.date()==vortex->getTime().date()) {
	int obTimeDiff = time.time().secsTo(vortex->getTime().time());
	if ((obTimeDiff > 0) and (obTimeDiff <= maxObTimeDiff)) {
	  // Check the distance
	  float vortexLat = vortex->getLat(heightIndex);
	  float vortexLon = vortex->getLon(heightIndex);
	  float obLat = pressureList->at(i).getLat();
	  float obLon = pressureList->at(i).getLon();
	  float* relDist = gridData->getCartesianPoint(&vortexLat, &vortexLon,
						       &obLat, &obLon);
	  float obRadius = sqrt(relDist[0]*relDist[0] + relDist[1]*relDist[1]);
	  delete [] relDist;
	  
	  if ((obRadius >= vortex->getRMW(heightIndex)) and (obRadius <= maxObRadius)) {
	    // Good ob anchor!
	    presObs[numEstimates]=pressureList->at(i);
	    
	    float pPrimeOuter;
	    if (obRadius >= lastRing) {
	      pPrimeOuter = pD[(int)lastRing];
	    } else {
	      pPrimeOuter = pD[(int)obRadius];
	    }
	    float cpEstimate = obPressure - (pPrimeOuter - pD[0]);
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
	  } else if (obRadius < vortex->getRMW(heightIndex)) {
	    // Close enough to be called a central pressure
	    // Let PollThread handle that one
	  }
	
	  if(numEstimates > 100) {
	    log(Message(QString("Reached Pressure Estimate Limit"),0,this->objectName(),Yellow));
	    break;
	  }
	}
      }
    }
  }
  
  // Should have a sum of pressure estimates now, if not use 1013
  float avgPressure = 0;
  float avgWeight = 0;
  emit log(Message(QString("Number of Anchor Pressures Available "+QString().setNum(numEstimates)),0,this->objectName()));
  if (numEstimates > 1) {
    //Message::toScreen("Option 1");
    avgPressure = pressSum/pressWeight;
    avgWeight = pressWeight/(float)numEstimates;
    
		// Calculate the standard deviation of the estimates and set the global variables
    float sumSquares = 0;
    for (int i = 0; i < numEstimates; i++) {
      // Message::toScreen("Pressure Estimate "+QString().setNum(i)+" = "+QString().setNum(pressEstimates[i]));
      float square = weightEstimates[i] * (pressEstimates[i] - avgPressure) * (pressEstimates[i] - avgPressure);
      sumSquares += square;
    }

    centralPressure = avgPressure;
    
  } else if (numEstimates == 1) {
    // Can use a single pressure estimate but no standard deviation

    centralPressure = pressSum/pressWeight;
    	  
    emit log(Message(QString("Single anchor pressure only, using 2.5 hPa for uncertainty"),0,this->objectName()));
    
  } else {
    //Message::toScreen("Option 3");
    // Assume standard environmental pressure
    centralPressure = 1013 - (pD[(int)lastRing] - pD[0]);
    
    emit log(Message(QString("No anchor pressures, using 1013 hPa for environment and 5 hPa for uncertainty"),0,this->objectName()));
  }

  vortex->setPressure(centralPressure);
  //vortex->setPressureUncertainty(centralPressureStdDev);
  vortex->setPressureDeficit(pD[(int)lastRing]-pD[(int)firstRing]);
  
}

void VortexThread::catchLog(const Message& message)
{
  emit log(message);
}


void VortexThread::calcPressureUncertainty(float setLimit, QString nameAddition)
{
  // Calculates the uncertanty in pressure by perturbing the center of the vortex

  // Acquire vortexData center uncertainty for the second level we examined
  int goodLevel = 0;
  float height = vortexData->getHeight(goodLevel);
  float maxCoeffs = maxWave*2 + 3;
  float centerStd = vortexData->getCenterStdDev(goodLevel);
  
  //Message::toScreen("VortexThread: CalcPressureUncertainty: Uncertainty of center from vortexData is "+QString().setNum(centerStd));
  
  if(setLimit!=0) {
    centerStd = setLimit;
    //Message::toScreen("VortexThread: CalcPressureUncertainty: Uncertainty of center from vortexData is "+QString().setNum(centerStd));    
  }
  if(nameAddition!=QString())
    nameAddition = nameAddition+QString().setNum(centerStd);

  // Now move this amount of space in each cardinal direction to get 4 additional pressure estimates
  int numErrorPoints = 4;
  float angle = 2*acos(-1)/numErrorPoints;

  // Create extra vortexData to house uncertainty stuff
  QString file("vortrac_defaultVortexListStorage.xml");
  Configuration* vortexConfig = new Configuration(0, file);
  vortexConfig->setLogChanges(false);
  connect(vortexConfig, SIGNAL(log(const Message&)), 
	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  VortexList* errorVertices = new VortexList(vortexConfig);
  errorVertices->open();

  QDomElement vortexElem = configData->getConfig("vortex");
  QDomElement radarElem = configData->getConfig("radar");
  QString vortexPath = configData->getParam(vortexElem, "dir");
  QString vortexName = configData->getParam(vortexElem, "name");
  errorVertices->setVortexName(vortexName);
  QString radarName = configData->getParam(radarElem,"name");
  errorVertices->setRadarName(radarName);
  errorVertices->setProductType("Uncertainty in Pressure");
  QString timeString = vortexData->getTime().toString("yyyy_MM_ddThh_mm_ss");
  
  QDir workingDirectory(vortexPath);
  if(nameAddition!=QString()) {

    // Output of the uncertainty will save if the file name addition is not 
    // empty - Testing and analysis of methods only

    workingDirectory.mkdir("pError_"+nameAddition+"_"+timeString);
    QString outFileName = workingDirectory.path()+"/";
    outFileName += vortexName+"_"+radarName+"_"+timeString+"_"+nameAddition+".xml";
    errorVertices->setFileName(outFileName);
    errorVertices->setNewWorkingDirectory(workingDirectory.path()+"/pError_"+nameAddition+"_"+timeString+"/");
    errorVertices->save();
  }

  // Add the current vortexData to the errorVertices for comparison purposes
  // Be careful how we handle this point in subsequent averages
  //errorVertices->setNewWorkingDirectory(workingDirectory.path()+"/uncertainty+"+QString().setNum(0)+"/");
  //workingDirectory.mkdir("uncertainty"+QString().setNum(0)+"/");
  errorVertices->append(*vortexData);
  if(nameAddition!=QString())
    errorVertices->save();
  
  emit log(Message(QString(),2,this->objectName()));  // 82 %
  
  // Create a GBVTD object to process the rings
  if(closure.contains(QString("hvvp"),Qt::CaseInsensitive)) {
    if(calcHVVP(false)) {
      vtd = new GBVTD(geometry, closure, maxWave, dataGaps, hvvpResult);
      emit log(Message(QString(),0,this->objectName(),Green));
    }
    else{
      emit log(Message(QString(),0,this->objectName(),Yellow,
		       QString("HVVP Failure: Could Not Retrieve HVVP Winds")));
      vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
    }
  }
  else {
    vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
  }
  vtdCoeffs = new Coefficient[20];

  int loopPercent = int(15.0/float(numErrorPoints));
  int endPercent = 15-(numErrorPoints*loopPercent);
  
  
  float refLat = vortexData->getLat(goodLevel);
  float refLon = vortexData->getLon(goodLevel);
  float sqDeficitSum = 0;
  for(int p = 0; p < numErrorPoints; p++) {
    VortexData* errorVertex = new VortexData(1,vortexData->getNumRadii(), vortexData->getNumWaveNum());
    errorVertex->setTime(vortexData->getTime().addDays(p).addYears(2));
    
    // Set the reference point
    float* newLatLon = gridData->getAdjustedLatLon(refLat,refLon,
						   centerStd*cos(p*angle),centerStd*sin(p*angle));
    errorVertex->setLat(0,newLatLon[0]); 
    errorVertex->setLon(0,newLatLon[1]);
    errorVertex->setHeight(0,height);
    gridData->setAbsoluteReferencePoint(newLatLon[0], newLatLon[1], height);
    delete  [] newLatLon;

    emit log(Message(QString(),loopPercent,this->objectName()));

    if ((gridData->getRefPointI() < 0) || 
	(gridData->getRefPointJ() < 0) ||
	(gridData->getRefPointK() < 0)) {
      // Out of bounds problem
      emit log(Message(QString("Error Vertex is outside CAPPI"),0,
		       this->objectName()));
      continue;
    }			
    
    for (float radius = firstRing; radius <= lastRing; radius++) {
      
      // Get the cartesian points
      float xCenter = gridData->getCartesianRefPointI();
      float yCenter = gridData->getCartesianRefPointJ();
      
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
	  emit log(Message(QString("CalcPressureUncertainty:Error retrieving VTC0 in vortex!"),0,this->objectName()));
	} 
      } else {
	QString err("Insufficient data for VTD winds at radius ");
	QString loc;
	err.append(loc.setNum(radius));
	err.append(", height ");
	err.append(loc.setNum(height));
	emit log(Message(err));
      }
      
      // All done with this radius and height, archive it
      int vertexLevel = 0;
      archiveWinds(*errorVertex, radius, vertexLevel, maxCoeffs);

      // Clean up
      delete[] ringData;
      delete[] ringAzimuths;
    }

    // Now calculate central pressure for each of these
    float* errorPressureDeficit = new float[(int)lastRing+1];
    getPressureDeficit(errorVertex,errorPressureDeficit, height);
    errorVertex->setPressureDeficit(fabs(*errorPressureDeficit));

    // Sum for Deficit Uncertainty
    sqDeficitSum += (errorVertex->getPressureDeficit()-vortexData->getPressureDeficit())*(errorVertex->getPressureDeficit()-vortexData->getPressureDeficit());
  
    // Add in uncertainty from multiple pressure measurements
    if(numEstimates < 1){
      // No outside data available use the 1013 bit.
      errorVertex->setPressure(1013 - (errorPressureDeficit[(int)lastRing] - errorPressureDeficit[0]));
      errorVertex->setPressureDeficit(errorPressureDeficit[(int)lastRing] - errorPressureDeficit[0]);
      //Message::toScreen("Adding uncertainty calib pressure "+QString().setNum(errorVertex->getPressure()));
      errorVertices->append(*errorVertex);
      if(nameAddition!=QString())
	errorVertices->save();
    }
    else {
      for(int j = 0; j < numEstimates; j++) {
	QString stationName = presObs[j].getStationName();
	QString obsTime = presObs[j].getTime().toString();
	float obPressure = presObs[j].getPressure();
	float vortexLat = errorVertex->getLat(0);
	float vortexLon = errorVertex->getLon(0);
	float obLat = presObs[j].getLat();
	float obLon = presObs[j].getLon();
	float* relDist = gridData->getCartesianPoint(&vortexLat, &vortexLon,
						     &obLat, &obLon);
	float obRadius = sqrt(relDist[0]*relDist[0] + relDist[1]*relDist[1]);
	delete [] relDist;
	float pPrimeOuter;
	if (obRadius >= lastRing) {
	  pPrimeOuter = errorPressureDeficit[(int)lastRing];
	} else {
	  pPrimeOuter = errorPressureDeficit[(int)obRadius]; 
	}
	//Message::toScreen("Calculating uncertainty calib "+QString().setNum(j)+" = "+QString().setNum(obPressure)+" @ "+QString().setNum(obRadius));
	errorVertex->setPressure(obPressure - (pPrimeOuter - errorPressureDeficit[0]));
	errorVertex->setPressureDeficit(errorPressureDeficit[(int)lastRing] - errorPressureDeficit[0]);

	VortexData* errorVertex2 = new VortexData(*errorVertex);
	errorVertex2->setTime(vortexData->getTime().addYears(2).addDays(p).addSecs((j+1)*60));
	errorVertices->append(*errorVertex2);
	QString obsStamp = stationName+"_"+obsTime;
	obsStamp+="_"+QString().setNum(obPressure);
	errorVertices->setIndividualProductType(errorVertices->count()-1,
						obsStamp);
	//Message::toScreen("Adding uncertainty calib pressure "+QString().setNum(errorVertex->getPressure()));
	
	if(nameAddition!=QString())
	  errorVertices->save();
	delete errorVertex2;
      }
    }
    delete [] errorPressureDeficit;
    delete errorVertex;
  }
  
  emit log(Message(QString(),endPercent, this->objectName()));
  
  delete [] vtdCoeffs;
  delete vtd;
  
  // Standard deviation from the center point
  float sqPressureSum = 0;
  for(int i = 1; i < errorVertices->count();i++) {
    sqPressureSum += pow((errorVertices->at(i).getPressure()-vortexData->getPressure()),2);
  }
  float pressureUncertainty = sqrt(sqPressureSum/(errorVertices->count()-2));
  float deficitUncertainty = sqrt(sqDeficitSum/(numErrorPoints-1));
  if((numEstimates <= 1)&&(pressureUncertainty < 2.5)) {
    pressureUncertainty = 2.5;
  }
  if(numEstimates == 0)
    pressureUncertainty = 5.0;

  vortexData->setPressureUncertainty(pressureUncertainty);
  vortexData->setDeficitUncertainty(deficitUncertainty);
  float aveRMWUncertainty = 0;
  int goodrmw = 0;
  for(int jj = 0; jj < vortexData->getNumLevels();jj++) {
    if(vortexData->getRMWUncertainty(jj) < (gridData->getIGridsp()/2)||
       vortexData->getRMWUncertainty(jj) < (gridData->getJGridsp()/2)) {
      if(gridData->getIGridsp()>=gridData->getJGridsp()) 
	vortexData->setRMWUncertainty(jj,gridData->getIGridsp()/2);
      else
	vortexData->setRMWUncertainty(jj,gridData->getJGridsp()/2);
    }
    if(fabs(vortexData->getRMWUncertainty(jj))<10 
       && vortexData->getRMW()!=-999 && vortexData->getRMW()>0) {
      goodrmw++;
      aveRMWUncertainty += vortexData->getRMWUncertainty();
    }
  }
  vortexData->setAveRMWUncertainty(aveRMWUncertainty/(1.0*goodrmw));

  //Message::toScreen(nameAddition+" uncertainty is "+QString().setNum(pressureUncertainty));
  errorVertices->takeFirst();
  errorVertices->prepend(*vortexData);
  if(nameAddition!=QString())
    errorVertices->save();
  delete errorVertices;

}

void VortexThread::readInConfig()
{
  QDomElement vtdConfig = configData->getConfig("vtd");
  QDomElement pressureConfig = configData->getConfig("pressure");

   vortexPath = configData->getParam(vtdConfig, 
				     QString("dir"));
   geometry = configData->getParam(vtdConfig,
				   QString("geometry"));
   refField =  configData->getParam(vtdConfig,
				    QString("reflectivity"));
   velField = configData->getParam(vtdConfig,
				   QString("velocity"));
   closure = configData->getParam(vtdConfig,
				  QString("closure"));
   
   firstLevel = configData->getParam(vtdConfig,
				     QString("bottomlevel")).toFloat();
   lastLevel = configData->getParam(vtdConfig,
				    QString("toplevel")).toFloat();
   firstRing = configData->getParam(vtdConfig,
				    QString("innerradius")).toFloat();
   lastRing = configData->getParam(vtdConfig, 
				   QString("outerradius")).toFloat();
   
   ringWidth = configData->getParam(vtdConfig, 
				    QString("ringwidth")).toFloat();
   maxWave = configData->getParam(vtdConfig, 
				  QString("maxwavenumber")).toInt();

   // Define the maximum allowable data gaps
   dataGaps = new float[maxWave+1];
   for (int i = 0; i <= maxWave; i++) {
     dataGaps[i] = configData->getParam(vtdConfig, 
					QString("maxdatagap"), QString("wavenum"), 
					QString().setNum(i)).toFloat();
   }

   // Set GriddedData to use ringwidth for spacing
   gridData->setCylindricalAzimuthSpacing(ringWidth);

  maxObRadius = 0;
  maxObTimeDiff = 60 * configData->getParam(pressureConfig, "maxobstime").toFloat();
  if(configData->getParam(pressureConfig, "maxobsmethod") == "center")
    maxObRadius = configData->getParam(pressureConfig, "maxobdist").toFloat();
  if(configData->getParam(pressureConfig, "maxobsmethod") == "ring")
    maxObRadius = lastRing+configData->getParam(pressureConfig, "maxobsdist").toFloat();

  if(maxObRadius == -999){
     maxObRadius = lastRing + 50;
  }
  if(maxObTimeDiff == -999){
     maxObTimeDiff = 59 * 60;
  }
  /*
  gradientHeight = configData->getParam(pressureConfig, "height").toFloat();
  Message::toScreen("Gradient Height is "+QString().setNum(gradientHeight)+" firstLevel is "+QString().setNum(firstLevel));
  if(gradientHeight < firstLevel) {
    QString message = QString(tr("Attempting to calculate pressure gradient below analyzed levels, using lowest analysis level ")+QString().setNum(firstLevel)+tr(" km"));
    emit log(Message(message, 0, this->objectName(),Yellow,
		     QString("Parameter Disagreement")));
    gradientHeight = firstLevel;
  }

 if(gradientHeight > lastLevel) {
    QString message = QString(tr("Attempting to calculate pressure gradient above analyzed levels, using highest analysis level ")+QString().setNum(firstLevel)+tr(" km"));
    emit log(Message(message, 0, this->objectName(),Yellow,
		     QString("Parameter Disagreement")));
    gradientHeight = lastLevel;
  }
  */
  gradientHeight = firstLevel;
}

bool VortexThread::calcHVVP(bool printOutput)
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

  int gradientIndex = vortexData->getHeightIndex(gradientHeight);
  QDomElement radar = configData->getConfig("radar");
  float radarLat = configData->getParam(radar,"lat").toFloat();
  float radarLon = configData->getParam(radar,"lon").toFloat();
  float vortexLat = vortexData->getLat(gradientIndex);
  float vortexLon = vortexData->getLon(gradientIndex);
  
  float* distance;
  distance = gridData->getCartesianPoint(&radarLat, &radarLon, 
					 &vortexLat, &vortexLon);
  float rt = sqrt(distance[0]*distance[0]+distance[1]*distance[1]);
  float cca = atan2(distance[0], distance[1])*180/acos(-1);
  delete [] distance;
  
  if(printOutput) {
    //Message::toScreen("Vortex (Lat,Lon): ("+QString().setNum(vortexLat)+", "+QString().setNum(vortexLon)+")");
    emit log(Message(QString("Vortex (Lat,Lon): ("+QString().setNum(vortexLat)+", "+QString().setNum(vortexLon)+")")));
    QString hvvpInput("Hvvp Parameters: Distance to Radar "+QString().setNum(rt)+" (km), angle to vortex center in degrees ccw from north "+QString().setNum(cca)+" (deg), rmw "+QString().setNum(vortexData->getAveRMW())+" km");
    emit log(Message(hvvpInput,0,this->objectName()));
    //Message::toScreen(hvvpInput);
  }
  
  Hvvp *envWindFinder = new Hvvp;
  envWindFinder->setConfig(configData);
  envWindFinder->setPrintOutput(printOutput);
  connect(envWindFinder, SIGNAL(log(const Message)), 
	  this, SLOT(catchLog(const Message)), 
	  Qt::DirectConnection);
  envWindFinder->setRadarData(radarVolume,rt, cca, vortexData->getAveRMW());
  emit log(Message(QString(), 1,this->objectName()));
  //envWindFinder->findHVVPWinds(false); for first fit only
  bool hasHVVP = envWindFinder->findHVVPWinds(true);
  hvvpResult = envWindFinder->getAvAcrossBeamWinds();
  hvvpUncertainty = envWindFinder->getAvAcrossBeamWindsStdError();
  if(isnan(hvvpResult)||(hvvpResult == -999)||
     isnan(hvvpUncertainty)||(hvvpUncertainty == -999)){
    hvvpResult = 0;
    hvvpUncertainty = 0;
    hasHVVP = false;
  }

  QString finalHVVP;
  if(printOutput && hasHVVP)
    finalHVVP = QString("Hvvp finds mean wind "+QString().setNum(hvvpResult)+" +/- "+QString().setNum(fabs(hvvpUncertainty)));
  
  emit log(Message(finalHVVP, 0,this->objectName()));
  delete envWindFinder;
  
  return hasHVVP;
}

