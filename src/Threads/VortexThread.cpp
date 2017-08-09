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
#include "VTD/mgbvtd.h"
#include "Math/Matrix.h"
#include "HVVP/Hvvp.h"

VortexThread::VortexThread(QObject *parent) : QObject(parent)
{
    this->setObjectName("GBVTD");

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

    // Claim and empty all the memory set aside
    gridData = NULL;
    vortexData = NULL;
    pressureList = NULL;
    configData = NULL;
    dataGaps = NULL;
}

VortexThread::~VortexThread()
{
    delete [] dataGaps;
}

void VortexThread::getWinds(Configuration *wholeConfig, GriddedData *dataPtr, RadarData *radarPtr,
			    VortexData* vortexPtr, PressureList *pressurePtr)
{
    pressureList = pressurePtr;
    // Set the grid object
    gridData = dataPtr;
    // Set the radar object
    radarVolume = radarPtr;
    // Set the vortex data object
    vortexData = vortexPtr;
    // Set the configuration info
    configData = wholeConfig;

    run();
}

void VortexThread::run()
{
    // Initialize variables
    readInConfig();

    // Set the output directory??
    //vortexResults.setNewWorkingDirectory(vortexPath);

    int maxCoeffs = maxWave * 2 + 3;

    // Find & Set Average RMW
    float rmw = 0;
    int goodrmw = 0;
    for(int l = 0; l < vortexData->getNumLevels(); l++) {
      if((vortexData->getRMW(l) != -999) && (vortexData->getRMW(l) != 0 )) {  // RMWs were set by ChooseCenter
            if(vortexData->getRMWUncertainty(l) < 10) {
                rmw += vortexData->getRMW(l);
                goodrmw++;
            }
        }
    }
    if (goodrmw)
        rmw = rmw/(1.0*goodrmw);

    vortexData->setAveRMW(rmw);
    // RMW is the average rmw taken over all levels of the vortexData

    // Create a GBVTD object to process the rings
    if(closure.contains(QString("hvvp"), Qt::CaseInsensitive)) {
        if(calcHVVP(true)) {
            vtd = new GBVTD(geometry, closure, maxWave, dataGaps, hvvpResult);
            //emit log(Message(QString(),6,this->objectName(),Green));
        }
        else {
            emit log(Message(QString(),5,this->objectName(),Yellow,QString("Could Not Retrieve HVVP Wind")));
            vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
        }
    }
    else {
        vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
        emit log(Message(QString(),5,this->objectName()));
    }
    Coefficient* vtdCoeffs = new Coefficient[20];

    // Placeholders for centers
    float xCenter = -999;
    float yCenter = -999;

    // TODO 7.0?
    
    int loopPercent = int(7.0 / float(gridData->getKdim()));
    // int endPercent = 7 - int(gridData->getKdim() * loopPercent);
    int storageIndex = -1;

    // TODO There seeme to be a disconnect between simplexData and vortexData
    //      vortexData is only going to have valid Lat and Lon for 0..n, where n is simplexData->getNumLevels()
    //      getNumLevels() returns a number set in SimplexThread::findCenter() and computed as
    //	                  int nTotalLevels = (int)floor((lastLevel - firstLevel) / zgridsp + 1.5);
    //      BUT lastLevel and firsLevel in this context come from the "center" section in the config file, not the "vtd" section.
    //      So for my KAMX example: firstLevel = 2, lastLevel = 7 in the "vtd" section,
    //            and these are the values used below to control the loop
    //            But vortexData is only going to have valid Lat/Lon from 0..3,
    //                because in "center" firstLevel is 2 and lastLevel is 3

    // HOW did this entire chunck of code get lost??? 
    
    // How do I get simplexDate->getNumLevels from here?
    int maxIndex = (lastLevel - firstLevel) / gridData->getKGridsp() ;
	
    for(storageIndex = 0; storageIndex <= maxIndex; storageIndex++) {
	// TODO: firstLevel and lastLevel come from the config file 
	//       getLat will return -999 if h is > 5 (hardcoded to MAXLEVELS)
	//       If _numLevel in the VortexData::get* refers to (toplevel - bottomlevel) in the config file <vtd> section
	
	// Set the reference point
        // float referenceLat = vortexData->getLat(h);
        // float referenceLon = vortexData->getLon(h);

	// This works, with storageIndex going from 0 to 12, height 4, 4.5, 5, ... 10
      
	// But from storage index 7 on, we are out of cappi box: Lat 16.5929, Lon -90.3586 for all entries.
	// how where these initialized? -> from ChooseCenter::_useLastMean()
	
        float referenceLat = vortexData->getLat(storageIndex);
        float referenceLon = vortexData->getLon(storageIndex);
	float height = firstLevel + storageIndex * gridData->getKGridsp();
#if 0	
	std::cout << "storageIndex: " << storageIndex << ", lat: " << referenceLat
		  << ", lon: " << referenceLon << ", height: " << height << std::endl;
#endif	
        gridData->setAbsoluteReferencePoint(referenceLat, referenceLon, height);
        if ((gridData->getRefPointI() < 0) || (gridData->getRefPointJ() < 0) ||(gridData->getRefPointK() < 0)) {
            emit log(Message(QString("Simplex center is outside CAPPI"), 0, this->objectName(), Yellow));
            continue;
        }
        
        // compute crossbeam wind to correct GBVTD result
        int gradientIndex = vortexData->getHeightIndex(gradientHeight);
        QDomElement radar = configData->getConfig("radar");
        float radarLat = configData->getParam(radar,"lat").toFloat();
        float radarLon = configData->getParam(radar,"lon").toFloat();
        float vortexLat = vortexData->getLat(gradientIndex);
        float vortexLon = vortexData->getLon(gradientIndex);

        float* distance = gridData->getCartesianPoint(&radarLat, &radarLon, &vortexLat, &vortexLon);
        float rt = sqrt(distance[0]*distance[0]+distance[1]*distance[1]);
        float cca = atan2(distance[0], distance[1])*180/acos(-1);
        delete [] distance;

        Hvvp *hvvp = new Hvvp;
        hvvp->setConfig(configData);
        hvvp->setRadarData(radarVolume, rt, cca, vortexData->getAveRMW());

        /* MGBVTD mgbvtd(gridData->getCartesianRefPointI(), 
                      gridData->getCartesianRefPointJ(),
                      height, vortexData->getAveRMW(), *gridData);
        float Vm = mgbvtd.computeCrossBeamWind(50.f, velField, vtd, hvvp); */
	float Vm = 0.0;

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
                    if(vtdCoeffs[0].getValue() != -999.f){
                        vtdCoeffs[0].setValue( vtdCoeffs[0].getValue()-Vm*radius/rt );
                    }
                } else {
                    emit log(Message(QString("Error retrieving VTC0 in vortex!"),0,this->objectName(), Yellow));
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
            archiveWinds(radius, storageIndex, maxCoeffs, vtdCoeffs);
        }
    }
    emit log(Message(QString(),15,this->objectName()));

    // Clean up
    delete vtd;

    // Integrate the winds to get the pressure deficit at the 2nd level (presumably 2km)  TODO Asumption on level here.
    
    float* pressureDeficit = new float[ (int) lastRing + 1];
    getPressureDeficit(vortexData, pressureDeficit, gradientHeight);

    // Get the central pressure
    calcCentralPressure(vortexData, pressureDeficit, gradientHeight); // is firstLevel right?
    if (vortexData->getPressure() != -999.0) {
        calcPressureUncertainty(1, QString());
    } else {
        vortexData->setPressureUncertainty(-999.0);
        vortexData->setDeficitUncertainty(-999.0);
        vortexData->setAveRMWUncertainty(-999.0);
    }
	
    // Get the estimated surface wind
    getMaxSfcWind(vortexData);
	
    delete [] vtdCoeffs;
    delete [] pressureDeficit;
}

void VortexThread::archiveWinds(float radius, int hIndex, int maxCoeffs, Coefficient* vtdCoeffs)
{
    // Save the centers to the VortexData object
    int level = hIndex;
    int ring = int(radius - firstRing);

    for (int coeff = 0; coeff < maxCoeffs; coeff++) {
        vortexData->setCoefficient(level, ring, coeff, vtdCoeffs[coeff]);
        Coefficient current = vtdCoeffs[coeff];

	// DEBUG
	// std::cout << "level: " << current.getLevel()
	// 	  << ", radius: " << current.getRadius() << ", value: " << current.getValue()
	// 	  << ", param: " << current.getParameter().toLatin1().data()
	// 	  << std::endl;
	
	if((current.getValue() != -999) && (current.getValue() != 0)) {
            if(current.getRadius() > vortexData->getMaxValidRadius())
                vortexData->setMaxValidRadius(current.getRadius());
        }
    }
}

void VortexThread::archiveWinds(VortexData& data, float& radius, int& hIndex, int& maxCoeffs, Coefficient* vtdCoeffs)
{
    // Save the centers to the VortexData object
    int level = hIndex;
    int ring = int(radius - firstRing);

    for (int coeff = 0; coeff < maxCoeffs; coeff++) {
        data.setCoefficient(level, ring, coeff, vtdCoeffs[coeff]);
    }
}

void VortexThread::getPressureDeficit(VortexData* data, float* pDeficit,const float& height)
{
    float* dpdr = new float[ (int) lastRing + 1];

    // get the index of the height we want
    int heightIndex = data->getHeightIndex(height);

    // Assuming radius is in KM here, when we correct for units later change this
    float deltar = 1000;

    // Initialize arrays
    for (float radius = 0; radius <= lastRing; radius++) {
        dpdr[(int)radius] = -999;
        pDeficit[(int)radius] = 0;
    }

    // Get coriolis parameter
    float f = 2 * 7.29e-5 * sin(data->getLat(heightIndex) * 3.141592653589793238462643 / 180.);

    for (float radius = firstRing; radius <= lastRing; radius++) {
      // if (!(data->getCoefficient(height, radius, QString("VTC0")) == Coefficient())) {
      if ( (data->getCoefficient(height, radius, QString("VTC0"))).isValid()) {
            float meanVT = data->getCoefficient(height, radius, QString("VTC0")).getValue();
            if (meanVT != 0) {
                dpdr[(int)radius] = ((f * meanVT) + (meanVT * meanVT)/(radius * deltar)) * rhoBar[ (int) height - 1];
            }
        }
    }
    if (dpdr[(int)lastRing] != -999) {
        pDeficit[(int)lastRing] = -dpdr[(int)lastRing] * deltar * 0.001;
    }

    for (float radius = lastRing-1; radius >= 0; radius--) {
        if (radius >= firstRing) {
            if ((dpdr[(int)radius] != -999) and (dpdr[(int)radius+1] != -999)) {
                pDeficit[(int)radius] = pDeficit[(int)radius+1] -(dpdr[(int)radius] + dpdr[(int)radius+1]) * deltar * 0.001/2;
            } else if (dpdr[(int)radius] != -999) {
                pDeficit[(int)radius] = pDeficit[(int)radius+1] -dpdr[(int)radius] * deltar * 0.001;
            } else if (dpdr[(int)radius+1] != -999) {
                pDeficit[(int)radius] = pDeficit[(int)radius+1] -dpdr[(int)radius+1] * deltar * 0.001;
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
    float pressureDeficit = pD[(int)lastRing] - pD[0];
    if (pressureDeficit == 0) {
        // No analysis, so return -999
        vortex->setPressure(-999);
        vortex->setPressureDeficit(-999);
        return;
    }
    
    // Sum values to hold pressure estimates
    float pressWeightSum = 0;
    float pressSum = 0;
    numEstimates = 0;
    float* pressEstimates = new float[pressureList->size()];
    float* weightEstimates = new float[pressureList->size()];
    
    // Iterate through the pressure data
    //Message::toScreen("Size of searching List = "+QString().setNum(pressureList->size())+" within time "+QString().setNum(maxObTimeDiff)+" of vortex time "+vortex->getTime().toString(Qt::ISODate));
    for (int i = 0; i < pressureList->size(); i++) {
        float obPressure = pressureList->at(i).getPressure();
        if (obPressure > 0) {
            // Check the time
            QString obTime = pressureList->at(i).getTime().toString(Qt::ISODate);
            QString vortexTime = vortex->getTime().toString(Qt::ISODate);
            int obTimeDiff = pressureList->at(i).getTime().secsTo(vortex->getTime());
            if ((obTimeDiff > 0) and (obTimeDiff <= maxObTimeDiff)) {
                // Check the distance
                float vortexLat = vortex->getLat(heightIndex);
                float vortexLon = vortex->getLon(heightIndex);
                float obLat = pressureList->at(i).getLat();
                float obLon = pressureList->at(i).getLon();
                float* relDist = gridData->getCartesianPoint(&vortexLat, &vortexLon,&obLat, &obLon);
                float obRadius = sqrt(relDist[0]*relDist[0] + relDist[1]*relDist[1]);
                delete [] relDist;
                if ((obRadius >= 20) and (obRadius <= maxObRadius)) {
                //if ((obRadius >= vortex->getRMW(heightIndex)) and (obRadius <= maxObRadius)) {
                    // Good ob anchor!
                    _presObs.append(pressureList->at(i));
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
                    pressWeightSum += weight;
                    pressSum += (weight * cpEstimate);
                    pressEstimates[numEstimates] = cpEstimate;
                    weightEstimates[numEstimates] = weight;
                    numEstimates++;
                    
                }
            }
        }
    }

    if (envPressure != -999) {
        float pPrimeOuter;
        if (maxObRadius >= lastRing) {
            pPrimeOuter = pD[(int)lastRing];
        } else {
            pPrimeOuter = pD[(int)maxObRadius];
        }
        float cpEstimate = envPressure - (pPrimeOuter - pD[0]);
        float weight = 1.0;
        
        // Sum the estimate and save the value for Std Dev calculation
        pressWeightSum += weight;
        pressSum += (weight * cpEstimate);
        pressEstimates[numEstimates] = cpEstimate;
        weightEstimates[numEstimates] = weight;
        numEstimates++;

    }
    if (numEstimates > 0)
        centralPressure = pressSum/pressWeightSum;
    else
        centralPressure = 1013 - (pD[(int)lastRing] - pD[0]);

    vortex->setPressure(centralPressure);
    vortex->setPressureDeficit(pD[(int)lastRing]-pD[(int)firstRing]);

    delete[] pressEstimates;
    delete[] weightEstimates;
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
    int   maxCoeffs = maxWave * 2 + 3;
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
    float angle = 2 * acos(-1) / numErrorPoints;

    // Create a GBVTD object to process the rings
    if(closure.contains(QString("hvvp"), Qt::CaseInsensitive)) {
        if(calcHVVP(false)) {
            vtd = new GBVTD(geometry, closure, maxWave, dataGaps, hvvpResult);
            emit log(Message(QString(), 0, this->objectName(), Green));
        }
        else{
            emit log(Message(QString(), 0, this->objectName(), Yellow,
                             QString("Could Not Retrieve HVVP Wind")));
            vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
        }
    }
    else {
        vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
    }
    
    Coefficient* vtdCoeffs = new Coefficient[20];
    VortexList errorVertices;
    float refLat = vortexData->getLat(goodLevel);
    float refLon = vortexData->getLon(goodLevel);
    float sqDeficitSum = 0;
    
    for(int p = 0; p < numErrorPoints; p++) {
        VortexData* errorVertex = new VortexData(1, vortexData->getNumRadii(), vortexData->getNumWaveNum());
        errorVertex->setTime(vortexData->getTime().addDays(p).addYears(2));
        errorVertex->setHeight(0, vortexData->getHeight(goodLevel));
        // Set the reference point
        float* newLatLon = gridData->getAdjustedLatLon(refLat, refLon,
						       centerStd * cos(p * angle),
						       centerStd * sin(p * angle));
        gridData->setAbsoluteReferencePoint(newLatLon[0], newLatLon[1], height);
        delete  [] newLatLon;

        if ((gridData->getRefPointI() < 0) || (gridData->getRefPointJ() < 0) || (gridData->getRefPointK() < 0)) {
            // Out of bounds problem
            emit log(Message(QString("Error Vertex is outside CAPPI"), 0, this->objectName()));
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
            if (vtd->analyzeRing(xCenter, yCenter, radius, height, numData, ringData, ringAzimuths, vtdCoeffs, vtdStdDev)) {
                if (vtdCoeffs[0].getParameter() != "VTC0") {
                    emit log(Message(QString("CalcPressureUncertainty:Error retrieving VTC0 in vortex!"), 0, this->objectName()));
                }

                // All done with this radius and height, archive it
                int vertexLevel = 0;
                archiveWinds(*errorVertex, radius, vertexLevel, maxCoeffs, vtdCoeffs);

                // Clean up
                delete[] ringData;
                delete[] ringAzimuths;
            }
        }
        // Now calculate central pressure for each of these
        float* errorPressureDeficit = new float[(int)lastRing + 1];
        getPressureDeficit(errorVertex,errorPressureDeficit, height);
        errorVertex->setPressureDeficit(fabs(*errorPressureDeficit));

        // Sum for Deficit Uncertainty
        sqDeficitSum += (errorVertex->getPressureDeficit() - vortexData->getPressureDeficit())
	  * (errorVertex->getPressureDeficit() - vortexData->getPressureDeficit());

        // Add in uncertainty from multiple pressure measurements
        if(_presObs.size() < 1){
            // No outside data available use the 1013 bit.
            errorVertex->setPressure(1013 - (errorPressureDeficit[(int)lastRing] - errorPressureDeficit[0]));
            errorVertex->setPressureDeficit(errorPressureDeficit[(int)lastRing] - errorPressureDeficit[0]);
        }
        else {
            for(int j = 0; j < _presObs.size(); j++) {

                float obPressure = _presObs[j].getPressure();
                float vortexLat = errorVertex->getLat(0);
                float vortexLon = errorVertex->getLon(0);
                float obLat = _presObs[j].getLat();
                float obLon = _presObs[j].getLon();
                float* relDist = gridData->getCartesianPoint(&vortexLat, &vortexLon, &obLat, &obLon);
                float obRadius = sqrt(relDist[0] * relDist[0] + relDist[1] * relDist[1]);
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
            }
        }
        errorVertices.append(*errorVertex);
        delete [] errorPressureDeficit;
        delete errorVertex;
    }

    delete[] vtdCoeffs;
    delete vtd;

    // Standard deviation from the center point
    float sqPressureSum = 0;
    for(int i = 1; i < errorVertices.count();i++) {
        float prsDelta=errorVertices.at(i).getPressure() - vortexData->getPressure();
        sqPressureSum += pow(prsDelta, 2);
    }
    float pressureUncertainty = sqrt(sqPressureSum / (errorVertices.count() - 2));
    float deficitUncertainty = sqrt(sqDeficitSum / (numErrorPoints - 1));
    if((numEstimates <= 1)&&(pressureUncertainty < 2.5)) {
        pressureUncertainty = 2.5;
    }
    if(numEstimates == 0)
        pressureUncertainty = 5.0;
    vortexData->setPressureUncertainty(pressureUncertainty);
    vortexData->setDeficitUncertainty(deficitUncertainty);

    float aveRMWUncertainty = 0;
    int goodrmw = 0;
    for(int jj = 0; jj < vortexData->getNumLevels(); jj++) {
        if(vortexData->getRMWUncertainty(jj) < (gridData->getIGridsp() / 2)
	   ||vortexData->getRMWUncertainty(jj) < (gridData->getJGridsp() / 2)) {
            if(gridData->getIGridsp() >= gridData->getJGridsp())
                vortexData->setRMWUncertainty(jj, gridData->getIGridsp() / 2);
            else
                vortexData->setRMWUncertainty(jj, gridData->getJGridsp() / 2);
        }
        if(fabs(vortexData->getRMWUncertainty(jj)) < 10
                && vortexData->getRMW()!= -999 && vortexData->getRMW() > 0) {
            goodrmw++;
            aveRMWUncertainty += vortexData->getRMWUncertainty();
        }
    }
    vortexData->setAveRMWUncertainty(aveRMWUncertainty / (1.0 * goodrmw));
}

void VortexThread::readInConfig()
{
    QDomElement vtdConfig = configData->getConfig("vtd");
    QDomElement pressureConfig = configData->getConfig("pressure");

    vortexPath = configData->getParam(vtdConfig,QString("dir"));
    geometry = configData->getParam(vtdConfig,QString("geometry"));
    refField =  configData->getParam(vtdConfig,QString("reflectivity"));
    velField = configData->getParam(vtdConfig,QString("velocity"));
    closure = configData->getParam(vtdConfig,QString("closure"));

    firstLevel = configData->getParam(vtdConfig,QString("bottomlevel")).toFloat();
    lastLevel  = configData->getParam(vtdConfig,QString("toplevel")).toFloat();

    firstRing = configData->getParam(vtdConfig,QString("innerradius")).toFloat();
    lastRing  = configData->getParam(vtdConfig,QString("outerradius")).toFloat();

    ringWidth = configData->getParam(vtdConfig,QString("ringwidth")).toFloat();
    maxWave = configData->getParam(vtdConfig,QString("maxwavenumber")).toInt();

    // Define the maximum allowable data gaps
    dataGaps = new float[maxWave+1];
    for (int i = 0; i <= maxWave; i++) {
        dataGaps[i] = configData->getParam(vtdConfig, QString("maxdatagap"), QString("wavenum"),QString().setNum(i)).toFloat();
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
    gradientHeight = firstLevel;
    envPressure = -999;
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
    float rt = sqrt(distance[0] * distance[0] + distance[1]  *distance[1]);
    float cca = atan2(distance[0], distance[1]) * 180 / acos(-1);
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
    connect(envWindFinder, SIGNAL(log(const Message)),this, SLOT(catchLog(const Message)));
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

void VortexThread::getMaxSfcWind(VortexData* data)
{
    float xCenter = -999;
    float yCenter = -999;
    double maxwind = 0;
    double xmax = 0;
    double ymax = 0;

    int maxIndex = (lastLevel - firstLevel) / gridData->getKGridsp() ;
	
    for(int storageIndex = 0; storageIndex <= maxIndex; storageIndex++) {
      
        float referenceLat = vortexData->getLat(storageIndex);
        float referenceLon = vortexData->getLon(storageIndex);
	float height = firstLevel + storageIndex * gridData->getKGridsp();

	if ( (referenceLat == -999) || (referenceLon == -999) )
	  continue;
	
	// Set the reference point
	
        gridData->setAbsoluteReferencePoint(referenceLat, referenceLon, height);
        if ((gridData->getRefPointI() < 0) || (gridData->getRefPointJ() < 0) || (gridData->getRefPointK() < 0)) {
            emit log(Message(QString("Simplex center is outside CAPPI"),0,this->objectName(),Yellow));
            continue;
        }

        for (float radius = firstRing; radius <= lastRing; radius++) {
			
	  if(fabs(radius - vortexData->getAveRMW()) > 20) continue;
            // Get the cartesian points
            xCenter = gridData->getCartesianRefPointI();
            yCenter = gridData->getCartesianRefPointJ();

	    // Get thetaT
	    float thetaT = atan2(yCenter, xCenter);
	    thetaT = fixAngle(thetaT);
	    // float centerDistance = sqrt(xCenter * xCenter + yCenter * yCenter);
			
            // Get the winds
	    // if (!(data->getCoefficient(height, radius, QString("VTC0")) == Coefficient())) {
	    if ( (data->getCoefficient(height, radius, QString("VTC0"))).isValid()) {
	      
	      float vtc0 = data->getCoefficient(height, radius, QString("VTC0")).getValue();
	      float vrc0 = data->getCoefficient(height, radius, QString("VRC0")).getValue();
	      float vmc0 = data->getCoefficient(height, radius, QString("VMC0")).getValue();
	      float vtc1 = data->getCoefficient(height, radius, QString("VTC1")).getValue();
	      float vts1 = data->getCoefficient(height, radius, QString("VTS1")).getValue();
	      double PI = acos(-1.0);
	      
	      for (int i = 0; i < 360; i++) {
		// Convert to Psi
		float azimuth = PI * float(i) / 180.0;
		float angle = azimuth - thetaT;
		angle = fixAngle(angle);
		float xx = xCenter + radius * cos(angle + thetaT);
		float yy = yCenter + radius * sin(angle + thetaT);
		float psiCorrection = atan2(yy, xx) - thetaT;
		float psi = angle - psiCorrection;
		psi = fixAngle(psi);
		double vt = 0;
		double vr = 0;
		double vm = 0;
		if (vtc0 != -999) {
		  vt += vtc0;
		}
		if ((vtc1 != -999) and (vts1 != -999)) {
		  vt += vtc1*cos(psi) + vts1*sin(psi);
		}
		if (vrc0 != -999) {
		  vr = vrc0;
		}
		if (vmc0 != -999) {
		  vm = vmc0;
		}					
		double u = -vt * sin(azimuth) + vr * cos(azimuth) + vm * cos(angle);
		double v =  vt * cos(azimuth) + vr * sin(azimuth) + vm * sin(angle);
		double wspd = sqrt(u * u + v * v);
		if (wspd > maxwind) {
		  maxwind = wspd;
		  xmax = xx;
		  ymax = yy;
		}
	      }
	    }
	}
    }
    data->setMaxSfcWind(maxwind * 1.9438445 * 0.90); 
    // 1.652267825 multiplier if reducing by 85%;
}


float VortexThread::fixAngle(float& angle)
{
  // Make sure an angle is between 0 and 2Pi
  double PI = acos(-1.0);
  if (fabs(angle) < 1.0e-06) angle = 0.0;
  if (angle > (2*PI)) angle = angle - 2 * PI;
  if (angle < 0.) angle = angle + 2 * PI;
  return angle;
}

