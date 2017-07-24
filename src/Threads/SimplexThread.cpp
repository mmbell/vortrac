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

// TODO debug
# include <iostream>

SimplexThread::SimplexThread(QObject* parent):QObject(parent)
{
    this->setObjectName("Simplex");
    velNull = -999.;
    gridData = NULL;
    configData = NULL;

    _dataGaps = NULL;
    _vtdCoeffs = NULL;
}

SimplexThread::~SimplexThread()
{
    delete   _simplexVTD;
    delete[] _vtdCoeffs;
    delete[] _dataGaps;
}

void SimplexThread::initParam(Configuration *wholeConfig,GriddedData *dataPtr,float latGuess,float lonGuess)
{

    // Set the grid object
    gridData = dataPtr;

    // Set simplex initial guess
    _latGuess = latGuess;
    _lonGuess = lonGuess;

    // Set the configuration info
    configData = wholeConfig;
}

bool SimplexThread::findCenter(SimplexList* simplexList)
{   

    //STEP 1: retrieval all the parameters for Simplex algorithm
    QDomElement simplexCfg = configData->getConfig("center");
    QString geometry = configData->getParam(simplexCfg,QString("geometry"));
    QString velField = configData->getParam(simplexCfg,QString("velocity"));
    QString closure = configData->getParam(simplexCfg,QString("closure"));

    firstLevel= configData->getParam(simplexCfg,QString("bottomlevel")).toFloat();
    lastLevel = configData->getParam(simplexCfg,QString("toplevel")).toFloat();
    firstRing = configData->getParam(simplexCfg,QString("innerradius")).toFloat();
    lastRing  = configData->getParam(simplexCfg,QString("outerradius")).toFloat();

    float boxSize = configData->getParam(simplexCfg,QString("boxdiameter")).toFloat();
    float numPoints = configData->getParam(simplexCfg,QString("numpoints")).toFloat();

    if(numPoints >= 25) {
      std::cerr << "*** Error: <numpoints> is greater than 25 (the size of the data structures in the simplex thread)" << std::endl;
      return false;
    }
    
    float boxRowLength = sqrt(numPoints);
    float boxIncr = boxSize / sqrt(numPoints);

    float radiusOfInfluence = configData->getParam(simplexCfg,QString("influenceradius")).toFloat();
    float convergeCriterion = configData->getParam(simplexCfg,QString("convergence")).toFloat();
    float maxIterations = configData->getParam(simplexCfg,QString("maxiterations")).toFloat();
    float ringWidth = configData->getParam(simplexCfg,QString("ringwidth")).toFloat();
    int   maxWave = configData->getParam(simplexCfg,QString("maxwavenumber")).toInt();

    // Define the maximum allowable data gaps
    _dataGaps = new float[maxWave+1];
    for (int i = 0; i <= maxWave; i++) {
        _dataGaps[i] = configData->getParam(simplexCfg,QString("maxdatagap"), QString("wavenum"),QString().setNum(i)).toFloat();
    }

    //SETP 2: initialize a GBVTD object for whole simplex to use
    _simplexVTD = new GBVTD(geometry, closure, maxWave, _dataGaps);
    _vtdCoeffs  = new Coefficient[20];


    //STEP 3: perform simplex algorithm

    // Set ring width in cappi so that griddedData access function use this
    gridData->setCylindricalAzimuthSpacing(ringWidth);

    QDomElement cappi = configData->getConfig("cappi");
    float zgridsp = configData->getParam(cappi, "zgridsp").toFloat();
    int nTotalLevels = (int)floor((lastLevel - firstLevel) / zgridsp + 1.5);
    // We want 1 km spaced rings regardless of ring width
    int nTotalRings = (int)floor((lastRing - firstRing) + 1.5);
    // Create a simplexData object to hold the results;
    SimplexData* simplexData = new SimplexData(nTotalLevels, nTotalRings, (int)numPoints);

    // the number of levels should be divided by the zgridsp (cappi) because
    // those are measurements in km
    // the ring count should be divided by the ring width
    simplexData->setNumPointsUsed((int)numPoints);

    // Allocate memory for the vertices
    float** vertex = new float*[3];
    vertex[0 ]= new float[2];
    vertex[1] = new float[2];
    vertex[2] = new float[2];
    float* VT = new float[3];
    float* vertexSum = new float[2];

    // Loop through the levels and rings, Should this have some reference to grid spacing?
    
    for (float height = firstLevel; height <= lastLevel; height++) {
        for (float radius = firstRing; radius <= lastRing; radius++) {
            gridData->setAbsoluteReferencePoint(_latGuess, _lonGuess, height);
            // Set the corner of the box
            float CornerI = gridData->getCartesianRefPointI();
            float CornerJ = gridData->getCartesianRefPointJ();
	    
            // std::cout << "** ring: "<< radius <<" RefI: " << CornerI << " RefJ: "<< CornerJ << std::endl;
	    
            float RefK = gridData->getCartesianRefPointK();
            float RefI = CornerI;
            float RefJ = CornerJ;
	    
            if ((gridData->getRefPointI() < 0) || (gridData->getRefPointJ() < 0) || (gridData->getRefPointK() < 0))  {
                emit log(Message(QString("Initial simplex guess is outside CAPPI"),0,this->objectName()));
                archiveNull(simplexData, radius, height, numPoints);
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
	    // std::cout << "** Num of points: " << numPoints << std::endl;
	    
            for (int point = 0; point < numPoints; point++) {
                if (point < boxRowLength)
                    RefI = CornerI + float(point) * boxIncr;
                else
                    RefI = CornerI + float((point) % int(boxRowLength)) * boxIncr;

                RefJ = CornerJ + float(point / int(boxRowLength)) * boxIncr;

                startX[point] = RefI;
                startY[point] = RefJ;

                // Initialize vertices
                float sqr32 = 0.866025;
                vertex[0][0] = RefI;
                vertex[0][1] = RefJ + radiusOfInfluence;
                vertex[1][0] = RefI + sqr32 * radiusOfInfluence;
                vertex[1][1] = RefJ - 0.5 * radiusOfInfluence;
                vertex[2][0] = RefI - sqr32 * radiusOfInfluence;
                vertex[2][1] = RefJ - 0.5 * radiusOfInfluence;
                vertexSum[0] = 0;
                vertexSum[1] = 0;

                for (int v = 0; v <= 2; v++) {
                    //Calculate mean wind at each vertex
		  // TODO look at that GBVTD.cpp
                    VT[v] = _getSymWind(vertex[v][0], vertex[v][1], int(RefK), radius, height, velField);
                }

                // Run the simplex search loop
                float VTsolution = .0, Xsolution = 0. , Ysolution=0.;
                _getVertexSum(vertex, vertexSum);
                _centerIterate(vertex, vertexSum, VT, maxIterations, convergeCriterion, RefK, radius, height, velField,
                               VTsolution, Xsolution, Ysolution);

                // Done with simplex loop, should have values for the current point
                if ((VTsolution < 100.) and (VTsolution > 0.)) {
                    // Add to sum
                    meanXall  += Xsolution;
                    meanYall  += Ysolution;
                    meanVTall += VTsolution;
                    meanCount++;
                    // Add to array for storage
                    endX[point]  = Xsolution;
                    endY[point]  = Ysolution;
                    VTind[point] = VTsolution;
                } else {
                    endX[point]  = Center::_fillv;
                    endY[point]  = Center::_fillv;
                    VTind[point] = Center::_fillv;
                }
            }//point loop end

	    // std::cout << "Mean count before: " << meanCount << std::endl;
	    
            if (meanCount == 0) {
                archiveNull(simplexData, radius, height, numPoints);
            } else {
                meanXall = meanXall / float(meanCount);
                meanYall = meanYall / float(meanCount);
                meanVTall = meanVTall / float(meanCount);
                for (int i = 0; i < numPoints; i++) {
                    if ((endX[i] != -999.) and (endY[i] != -999.) and (VTind[i] != -999.)) {
                        stdDevVertexAll += ((endX[i] - meanXall) * (endX[i] - meanXall) + (endY[i] - meanYall) * (endY[i] - meanYall));
                        stdDevVTAll += (VTind[i] - meanVTall) * (VTind[i] - meanVTall);
                    }
                }
                stdDevVertexAll = sqrt(stdDevVertexAll/float(meanCount - 1));
                stdDevVTAll = sqrt(stdDevVTAll/float(meanCount - 1));

                // Now remove centers beyond 1 standard deviation
                meanCount = 0;
                for (int i = 0; i < numPoints; i++) {
                    if ((endX[i] != -999.) and (endY[i] != -999.) and (VTind[i] != -999.)) {
                        float vertexDist = sqrt((endX[i] - meanXall) * (endX[i] - meanXall) + (endY[i] - meanYall) * (endY[i] - meanYall));
                        if (vertexDist < stdDevVertexAll) {
                            Xconv[meanCount] = endX[i];
                            Yconv[meanCount] = endY[i];
                            VTconv[meanCount] = VTind[i];
                            meanX += endX[i];
                            meanY += endY[i];
                            meanVT+= VTind[i];
                            meanCount++;
                        }
                    }
                }
		// std::cout << "Mean count after: " << meanCount << std::endl;
		
                if (meanCount == 0) {
                    archiveNull(simplexData, radius, height, numPoints);
                } else {
                    meanX = meanX / float(meanCount);
                    meanY = meanY / float(meanCount);
                    meanVT = meanVT / float(meanCount);
                    convergingCenters = meanCount;
                    for (int i = 0; i < convergingCenters - 1; i++) {
                        stdDevVertex += ((Xconv[i] - meanX) * (Xconv[i] - meanX)+ (Yconv[i] - meanY) * (Yconv[i] - meanY));
                        stdDevVT += (VTconv[i] - meanVT) * (VTconv[i] - meanVT);
                    }
                    stdDevVertex = sqrt(stdDevVertex / float(meanCount - 1));
                    stdDevVT = sqrt(stdDevVT / float(meanCount - 1));

                    // All done with this radius and height, archive it
                    archiveCenters(simplexData, radius, height, numPoints);
                }
            }
        }//ring loop end
    }//height loop end

    simplexList->append(*simplexData);
    delete simplexData;
    // Deallocate memory for the vertices
    delete[] vertex[0];
    delete[] vertex[1];
    delete[] vertex[2];
    delete[] vertex;
    delete[] VT;
    delete[] vertexSum;

    return true;
}

void SimplexThread::archiveCenters(SimplexData* simplexData, float radius, float height, float numPoints)
{
    // Save the centers to the SimplexData object
    int level =int(height - firstLevel);
    int ring  =int(radius - firstRing);
    simplexData->setHeight(level, height);
    simplexData->setRadius(ring, radius);
    simplexData->setMeanX(level, ring, meanX);
    simplexData->setMeanY(level, ring, meanY);
    simplexData->setMaxVT(level, ring, meanVT);
    simplexData->setCenterStdDev(level, ring, stdDevVertex);
    simplexData->setVTUncertainty(level, ring, stdDevVT);
    simplexData->setNumConvergingCenters(level, ring, (int)convergingCenters);
    for (int point = 0; point < (int)numPoints; point++) {
        // We want to use the real radius and height in the center for use
        // later so these should be given in km
        // The level and ring integers are the storage positions
        // these values are used for the indexing - LM 02/6/07
        Center indCenter(startX[point], startY[point], endX[point], endY[point], VTind[point], height, radius);
        simplexData->setCenter(level, ring, point, indCenter);
        simplexData->setInitialX(level, ring, point, startX[point]);
        simplexData->setInitialY(level, ring, point, startY[point]);
    }
}

void SimplexThread::archiveNull(SimplexData* simplexData, float& radius, float& height, float& numPoints)
{

    // Save the centers to the SimplexData object
    int level = int(height - firstLevel);
    int ring = int(radius - firstRing);
    simplexData->setHeight(level, height);
    simplexData->setRadius(ring, radius);
    simplexData->setMeanX(level, ring, -999);
    simplexData->setMeanY(level, ring, -999);
    simplexData->setMaxVT(level, ring, -999);
    simplexData->setCenterStdDev(level, ring, -999);
    simplexData->setVTUncertainty(level, ring, -999);
    simplexData->setNumConvergingCenters(level, ring, (int)0);
    for (int point = 0; point < (int)numPoints; point++) {
        Center indCenter(Center::_fillv, Center::_fillv, Center::_fillv,Center::_fillv,
                         Center::_fillv, level, ring);
        simplexData->setCenter(level, ring, point, indCenter);
    }

}

void SimplexThread::catchLog(const Message& message)
{
    emit log(message);
}

inline void SimplexThread::_getVertexSum(float** vertex,float* vertexSum)
{

    float sum;
    int v;
    for (int i=0; i<=1; i++) {
        for (sum = 0.0, v=0; v<=2; v++)
            sum += vertex[v][i];
        vertexSum[i] = sum;
    }
}

float SimplexThread::_simplexTest(float**& vertex,float*& VT,float*& vertexSum,
                                 float& radius, float& height, float& RefK,
                                 QString& velField, int& low, double factor)
{

    // Test a simplex vertex
    float VTtest = -999;
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
    if (_simplexVTD->analyzeRing(vertexTest[0], vertexTest[1], radius, height, numData, ringData,ringAzimuths, _vtdCoeffs, vtdStdDev)) {
        if (_vtdCoeffs[0].getParameter() == "VTC0") {
            VTtest = _vtdCoeffs[0].getValue();
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

float SimplexThread::_getSymWind(float vertex_x,float vertex_y,int RefK,float radius,float height,QString velField)
{
    float VT=-999.0f;
    gridData->setCartesianReferencePoint(int(vertex_x),int(vertex_y),RefK);
    int numData = gridData->getCylindricalAzimuthLength(radius, height);    // TODO
    float* ringData = new float[numData];
    float* ringAzimuths = new float[numData];
    gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
    gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);    // azimuth data should look like sine wave
#if 0
    // TODO debug
    for(int d = 0; d < numData; d++) {
      std::cout <<  "d: " << d << " val: " << ringData[d] 
		<< " azimuth: " << ringAzimuths[d] << std::endl;
    }
#endif
    Coefficient*  vtdCoeffs=new Coefficient[20];
    float   vtdStdDev;
    if (_simplexVTD->analyzeRing(vertex_x, vertex_y, radius, height, numData, ringData, ringAzimuths, vtdCoeffs, vtdStdDev)) {
        if (vtdCoeffs[0].getParameter() == "VTC0")
            VT = vtdCoeffs[0].getValue();
    }
    delete[] ringData;
    delete[] ringAzimuths;
    delete[] vtdCoeffs;
    return VT;
}

void SimplexThread::_centerIterate(float** vertex, float* vertexSum, float* VT, int maxIterations, float convergeCriterion,
                                   float RefK, float radius, float height, QString velField,
                                   float& VTsolution, float& Xsolution, float& Ysolution)
{
    VTsolution = Xsolution = Ysolution = 0.0f;

    int numIterations = 0;
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
            emit log(Message(QString("Maximum iterations exceeded in Simplex"),0,this->objectName()));
            break;
        }

        numIterations += 2;
        // Reflection
        float VTtest = _simplexTest(vertex, VT, vertexSum, radius, height,RefK, velField, low, -1.0);
        if (VTtest >= VT[high])
            // Better point than highest, so try expansion
            VTtest = _simplexTest(vertex, VT, vertexSum, radius, height,RefK, velField, low, 2.0);
        else if (VTtest <= VT[mid]) {
            // Worse point than second highest, so try contraction
            float VTsave = VT[low];
            VTtest = _simplexTest(vertex, VT, vertexSum, radius, height,RefK, velField, low, 0.5);
            if (VTtest <= VTsave) {
                for (int v=0; v<=2; v++) {
                    if (v != high) {
                        for (int i=0; i<=1; i++)
                            vertex[v][i] = vertexSum[i] = 0.5*(vertex[v][i] + vertex[high][i]);
                        VT[v]=_getSymWind(vertex[v][0],vertex[v][1],int(RefK),radius,height,velField);
                    }
                }
                numIterations += 2;
                _getVertexSum(vertex,vertexSum);
            }
        }
        else
            --numIterations;
    }
}

