/*
 * ChooseCenter.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/30/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "ChooseCenter.h"
#include "Math/Matrix.h"
#include <math.h>
#include <QDomElement>
#include <QHash>
#include <iostream>
#include <cstdlib>
#include <ctime>

ChooseCenter::ChooseCenter(Configuration* newConfig, const SimplexList* newList, VortexData* vortexPtr):
    MAX_ORDER(10),velNull(-999.0f)
{
    _config = newConfig;
    _simplexResults = newList;
    _vortexData = vortexPtr;
    _pppScore = NULL;
    _ppBestRadius = NULL;
    _ppBestFitVariance = NULL;
    _ppBestFitDegree = NULL;
    _pppBestFitCoeff = NULL;
    _ppNewBestRadius = NULL;
    _ppNewBestCenter = NULL;
    numHeights = NULL;
    indexOfHeights = NULL;
}

ChooseCenter::~ChooseCenter()
{
    if(_pppScore!=NULL) {
        for(int i = 0; i < _simplexResults->count(); i++) {
            for(int j = 0; j < _simplexResults->at(i).getNumRadii(); j++) {
                delete [] _pppScore[i][j];
            }
            delete [] _pppScore[i];
        }
        delete [] _pppScore;
    }
    else
        delete _pppScore;

    // We might not be able to delete these depending on weither they
    // are passed out for further use or not after the ChooseCenter object is
    // destroyed

    if(_ppBestRadius!=NULL) {
        for(int i = 0; i < _simplexResults->count(); i++)
            delete[] _ppBestRadius[i];
        delete[] _ppBestRadius;   }
    else
        delete _ppBestRadius;

    if(_ppNewBestRadius!=NULL) {
        for(int i = 0; i < _simplexResults->count(); i++)
            delete[] _ppNewBestRadius[i];
        delete[] _ppNewBestRadius; }
    else
        delete _ppNewBestRadius;

    if(_ppNewBestCenter!=NULL) {
        for(int i = 0; i < _simplexResults->count(); i++)
            delete[] _ppNewBestCenter[i];
        delete[] _ppNewBestCenter;
    }
    else
        delete _ppNewBestCenter;
    if(_pppBestFitCoeff!=NULL) {
        for(int m = 0; m < 4; m++) {
            for(int p = 0; p < numHeights->count(); p++)
                delete[] _pppBestFitCoeff[m][p];
            delete[] _pppBestFitCoeff[m];
        }
        delete[] _pppBestFitCoeff;
    }
    else
        delete _pppBestFitCoeff;

    if(_ppBestFitDegree!=NULL) {
        for(int m = 0; m < 4; m++)
            delete[] _ppBestFitDegree[m];
        delete[] _ppBestFitDegree;
    }
    else
        delete _pppBestFitCoeff;

    if(_ppBestFitVariance!=NULL) {
        for(int m = 0; m < 4; m++)
            delete[] _ppBestFitVariance[m];
        delete[] _ppBestFitVariance;
    }
    if(numHeights!=NULL)
        numHeights->clear();
    if(indexOfHeights!=NULL) {
        for(int ii = indexOfHeights->count()-1; ii >= 0; ii++) {
            int* trash = indexOfHeights->take(ii);
            delete [] trash;
        }
        indexOfHeights->clear();
    }
    delete numHeights;
    delete indexOfHeights;
}

bool ChooseCenter::findCenter(int level)
{
    /*
     * this function try to construct a polynomial fit of timeserial simplex data, at least 6 data points in
     * the recent 2 hours is needed to do this fitting. before performing the fitting, a average center is
     * compute, if the polynomial fitting fails or there is not enough history data, this average center is used.
     */
    _initialize();

    if(!_calMeanCenters()) {
        std::cerr << "Data Issues Could Not Find Mean Center" << std::endl;
        return false;
    }

    // TODO temporary
    _useLastMean();
    return true;
    
    // TODO: Why was this commented out?
    // polynomial fitting using data from last 2 hours, and volume number shoud > 6
    QList<int> validList;
    QDateTime lastTime = _simplexResults->last().getTime();
    for(int ii = (_simplexResults->size() - 1) ; ii >= 0; --ii) {
      if(_simplexResults->at(ii).getTime().secsTo(lastTime) < 2 * 60 * 60)
	validList.prepend(ii);
    }
    if(validList.size() > 6) {
      if (! _calPolyTest(validList, level) )   // TODO Why level 0?
	_useLastMean();
    }
    else
      _useLastMean();
    
    return true;
}

void ChooseCenter::_initialize()
{
    // Pulls all the necessary user parameters from the configuration panel
    //  and initializes the array used for fTesting

    QDomElement ccElement = _config->getConfig("choosecenter");
    _paramMinVolumes= _config->getParam(ccElement, QString("min_volumes")).toInt();
    _paramWindWeight= _config->getParam(ccElement, QString("wind_weight")).toFloat();
    _paramStdWeight = _config->getParam(ccElement, QString("stddev_weight")).toFloat();
    _paramPtsWeight = _config->getParam(ccElement, QString("pts_weight")).toFloat();

    _paramPosWeight = _config->getParam(ccElement, QString("position_weight")).toFloat();
    _paramRmwWeight = _config->getParam(ccElement, QString("rmw_weight")).toFloat();
    _paramVelWeight = _config->getParam(ccElement, QString("vt_weight")).toFloat();

    QDate sDate = QDate::fromString(_config->getParam(ccElement,QString("startdate")),Qt::ISODate);
    QDate eDate = QDate::fromString(_config->getParam(ccElement,QString("enddate")),Qt::ISODate);
    QTime sTime = QTime::fromString(_config->getParam(ccElement,QString("starttime")),Qt::ISODate);
    QTime eTime = QTime::fromString(_config->getParam(ccElement,QString("endtime")),Qt::ISODate);

    startTime = QDateTime(sDate, sTime, Qt::UTC);
    endTime = QDateTime(eDate, eTime, Qt::UTC);

    int fPercent = _config->getParam(ccElement, QString("stats")).toInt();
    if(fPercent == 99) {
        _fCriteria[0] = 4052.2;
        _fCriteria[1] = 98.50;
        _fCriteria[2] = 34.12;
        _fCriteria[3] = 21.20;
        _fCriteria[4] = 16.26;
        _fCriteria[5] = 13.75;
        _fCriteria[6] = 12.25;
        _fCriteria[7] = 11.26;
        _fCriteria[8] = 10.56;
        _fCriteria[9] = 10.04;
        _fCriteria[10] = 9.65;
        _fCriteria[11] = 9.33;
        _fCriteria[12] = 9.07;
        _fCriteria[13] = 8.86;
        _fCriteria[14] = 8.68;
        _fCriteria[15] = 8.53;
        _fCriteria[16] = 8.40;
        _fCriteria[17] = 8.29;
        _fCriteria[18] = 8.18;
        _fCriteria[19] = 8.10;
        _fCriteria[20] = 8.02;
        _fCriteria[21] = 7.95;
        _fCriteria[22] = 7.88;
        _fCriteria[23] = 7.82;
        _fCriteria[24] = 7.77;
        _fCriteria[25] = 7.72;
        _fCriteria[26] = 7.68;
        _fCriteria[27] = 7.64;
        _fCriteria[28] = 7.60;
        _fCriteria[29] = 7.56;

    } else {

	if (fPercent != 95)
	  std::cerr << "Unsupported <choosecenter><stats> value '" << fPercent << "'. Defaulting to 95" << std::endl;
    
        // fPercent = 95
        // these are our two options for now
        _fCriteria[0] = 161.45;
        _fCriteria[1] = 18.513;
        _fCriteria[2] = 10.128;
        _fCriteria[3] = 7.7086;
        _fCriteria[4] = 6.6079;
        _fCriteria[5] = 5.9874;
        _fCriteria[6] = 5.5914;
        _fCriteria[7] = 5.3177;
        _fCriteria[8] = 5.1174;
        _fCriteria[9] = 4.9646;
        _fCriteria[10] = 4.8443;
        _fCriteria[11] = 4.7472;
        _fCriteria[12] = 4.6672;
        _fCriteria[13] = 4.6001;
        _fCriteria[14] = 4.5431;
        _fCriteria[15] = 4.4940;
        _fCriteria[16] = 4.4513;
        _fCriteria[17] = 4.4139;
        _fCriteria[18] = 4.3808;
        _fCriteria[19] = 4.3513;
        _fCriteria[20] = 4.3248;
        _fCriteria[21] = 4.3009;
        _fCriteria[22] = 4.2793;
        _fCriteria[23] = 4.2597;
        _fCriteria[24] = 4.2417;
        _fCriteria[25] = 4.2252;
        _fCriteria[26] = 4.2100;
        _fCriteria[27] = 4.1960;
        _fCriteria[28] = 4.1830;
        _fCriteria[29] = 4.1709;
    }

    _ppBestRadius= new int*[_simplexResults->count()];
    _pppScore    = new float**[_simplexResults->count()];

    for(int i = 0; i < _simplexResults->count(); i++) {
        _pppScore[i] = new float*[_simplexResults->at(i).getNumRadii()];
        _ppBestRadius[i] = new int[_simplexResults->at(i).getNumLevels()];

#if 0
	std::cout << "At " << i << ", levels: " << _simplexResults->at(i).getNumLevels()
		  << ", radii: " << _simplexResults->at(i).getNumRadii() << std::endl;
#endif
	
        for(int rad = 0; rad < _simplexResults->at(i).getNumRadii(); rad++) {
            _pppScore[i][rad] = new float[_simplexResults->at(i).getNumLevels()];
            for(int level = 0; level < _simplexResults->at(i).getNumLevels(); level++){
                _pppScore[i][rad][level] = 0.0;
                _ppBestRadius[i][level] = -1;
            }
        }
    }
}

bool ChooseCenter::_calMeanCenters()
{
    /*
   * the result of Simplex contains k levels, each level has i rings. this function first
   * finds peak wind in those rings and give a score to each of them. then choose the ring
   * with a highest score as the ring of the level, then average all level to get a center
   * estimation of the volume. for each volume, do the same thing.
   * the score of each ring of all level from all volume are stored in (_pppScore[nVol][nLevel][nRing])
   * the index of best ring of each level from each volume is stored in (_ppBestRadius[nVol][nLevel])
   * the variation of center and radius of best ring if also stored in (_p
   *?but the mean center is not stored
   */
    if(_simplexResults->isEmpty())
        return false;

    for(int vidx = 0; vidx < _simplexResults->size(); vidx++) {
        float meanRadius = 0;
        float meanCenX = 0;
        float meanCenY = 0;
        const int NLEVEL = _simplexResults->at(vidx).getNumLevels();
        const int NRADII = _simplexResults->at(vidx).getNumRadii();
        for(int hidx = 0; hidx < NLEVEL; hidx++) {

            float *winds= new float[NRADII];
            float *stds = new float[NRADII];
            float *pts  = new float[NRADII];
            float bestWind = 0.0;
            float bestStd = 50.;
            float bestPts = 0.;
            float ptRatio = (float)_simplexResults->at(vidx).getNumPointsUsed() / 2.718281828;

            //get array of maxwind,centerSD,convegedPoints on this level, and calculate best value of these param
            for(int ridx = 0; ridx < NRADII; ridx++) {
                // Examine each radius based on index j, for the one containing the highest tangential winds

                winds[ridx] = _simplexResults->at(vidx).getMaxVT(hidx, ridx);
                stds[ridx]  = _simplexResults->at(vidx).getCenterStdDev(hidx, ridx);
                pts[ridx]   = _simplexResults->at(vidx).getNumConvergingCenters(hidx, ridx);

                if((winds[ridx] != SimplexData::_fillv) && (winds[ridx] > bestWind))
                    bestWind = winds[ridx];

                if((stds[ridx] != SimplexData::_fillv) && (stds[ridx] < bestStd))
                    bestStd = stds[ridx];

                if((pts[ridx] != SimplexData::_fillv) && (pts[ridx] > bestPts))
                    bestPts = pts[ridx];
            }

            // Formely known as fix winds which was a sub routine in the perl version of this algorithm
            // zeros all wind entrys that are not a local maxima, or adjacent to a local maxima
	    
            int count = 0;
            float *peakWinds = new float[NRADII];
            bool  *isPeaks   = new bool[NRADII];
	    
            for(int z = 0; z < NRADII; z++) {
                peakWinds[z] = 0.f;
                isPeaks[z] = 0;
            }
	    
            for(int a = 1; a < NRADII - 1; a++) {
                if((winds[a] >= winds[a-1]) && (winds[a] >= winds[a + 1])) {
                    peakWinds[count] = winds[a];
                    isPeaks[a] = 1;
                    count++;
                }
            }
            float peakWindMean = 0.f, peakWindStd = 0.f;
            for(int a = 0; a < count; a++) {
                peakWindMean += peakWinds[a];
            }
            if(count > 0) {
                peakWindMean = peakWindMean/((float)count);
                for(int z = 0; z < count; z++)
                    peakWindStd += (peakWinds[z] - peakWindMean) * (peakWinds[z] - peakWindMean);
                peakWindStd /= count;
            }
            delete[] peakWinds;

            //put point and points adjacent to peakwind into winds[]
            for(int jj = 0; jj < NRADII; jj++) {
                if(((jj > 0) && (jj < NRADII-1))
		   &&((isPeaks[jj] == 1) || (isPeaks[jj + 1] == 1) || (isPeaks[jj - 1] == 1))) {
                    winds[jj] = _simplexResults->at(vidx).getMaxVT(hidx, jj);
                    // Keep an eye out for the maxima
                    if(winds[jj] > bestWind) {
                        bestWind = winds[jj];
                    }
                }
                else {
                    winds[jj] = velNull;
                }
            }

            //calculate a weight for each ring, and find a best on this level
            float tempBest = 0.f, windScore, stdScore, ptsScore;
            int   bestFlag = 0;
            for(int rr = 0; rr < NRADII; rr++){
                windScore = stdScore = ptsScore = 0.f;
                _pppScore[vidx][rr][hidx] = velNull;
                if((bestWind != 0.0) && (winds[rr] != velNull))
                    windScore = exp(winds[rr] - bestWind) * _paramWindWeight;
                if((stds[rr] != velNull) && (stds[rr] != 0.0))
                    stdScore = bestStd / stds[rr] * _paramStdWeight;
                if((bestPts !=0 ) && (pts[rr] != velNull) && (ptRatio != 0.0)) {
                    ptsScore = log((float)pts[rr] / ptRatio) * _paramPtsWeight;
                }
                if(winds[rr] != velNull) {
                    _pppScore[vidx][rr][hidx] = windScore + stdScore + ptsScore;
                    if((_pppScore[vidx][rr][hidx] > tempBest) && (hidx >= 0) &&
		       (hidx <= _simplexResults->at(vidx).getNumLevels())) {
                        tempBest = _pppScore[vidx][rr][hidx];
                        bestFlag = rr;
                    }
                }
            }//end of radii
            meanRadius += _simplexResults->at(vidx).getRadius(bestFlag);
            meanCenX   += _simplexResults->at(vidx).getMeanX(hidx, bestFlag);
            meanCenY   += _simplexResults->at(vidx).getMeanY(hidx, bestFlag);
            _ppBestRadius[vidx][hidx] = bestFlag;

            delete [] winds;
            delete [] stds;
            delete [] pts;
            delete [] isPeaks;
        }//end of levels

        // calculate the mean radius and center scores over all levels
        meanRadius = meanRadius/NLEVEL;
        meanCenX   = meanCenX/NLEVEL;
        meanCenY   = meanCenY/NLEVEL;
        if(vidx == (_simplexResults->size() -1)){
            // const float radarLat = _config->getParam(_config->getConfig("radar"), QString("lat")).toFloat();
            // const float radarLon = _config->getParam(_config->getConfig("radar"), QString("lon")).toFloat();
            // const float radarLatRadians = radarLat * acos(-1.0) / 180.0;
            // const float fac_lat = 111.13209 - 0.56605 * cos(2.0 * radarLatRadians) + 0.00012 * cos(4.0 * radarLatRadians) - 0.000002 * cos(6.0 * radarLatRadians);
            // const float fac_lon = 111.41513 * cos(radarLatRadians) - 0.09455 * cos(3.0 * radarLatRadians) + 0.00012 * cos(5.0 * radarLatRadians);

            //std::cout<<"ChooseCenter found new mean center: "<<radarLat+meanCenY/fac_lat<<","<<radarLon+meanCenX/fac_lon<<","<<meanRadius<<std::endl;
        }
    }
    return true;
}

bool ChooseCenter::_calPolyTest(const QList<int>& volIdx,const int& levelIdx)
{
  bool retVal = true;
  
    const float radarLat = _config->getParam(_config->getConfig("radar"), QString("lat")).toFloat();
    const float radarLon = _config->getParam(_config->getConfig("radar"), QString("lon")).toFloat();
    const float radarLatRadians = radarLat * acos(-1.0)/180.0;
    const float fac_lat = 111.13209 - 0.56605 * cos(2.0 * radarLatRadians) + 0.00012 * cos(4.0 * radarLatRadians) - 0.000002 * cos(6.0 * radarLatRadians);
    const float fac_lon = 111.41513 * cos(radarLatRadians) - 0.09455 * cos(3.0 * radarLatRadians) + 0.00012 * cos(5.0 * radarLatRadians);


    const QDateTime firstTime =_simplexResults->at(volIdx[0]).getTime();
    const int nData = volIdx.size();
    float* xData = new float[nData];
    float* yData = new float[nData];

    //first get the xdata, which here is the time (in minute) from the firstTime
    for(int i = 0; i < volIdx.size(); ++i) {
        xData[i] = firstTime.secsTo(_simplexResults->at(volIdx[i]).getTime()) / 60.f;
    }

    //then retrieve ydata, we have 4 different ydata, so we'll process them one by one
    float** bestCoeff = new float*[4];
    for(int i = 0; i < 4; i++)
        bestCoeff[i] = new float[MAX_ORDER];
    float*  bestOrder = new float[4];
    float*  bestRSS = new float[4];

    for(int n = 0; n < 4; n++) {
        for(int i = 0; i < volIdx.size(); ++i) {
            int bestRadius =_ppBestRadius[volIdx[i]][levelIdx];
            switch(n) {
            case 0:
                yData[i] = _simplexResults->at(volIdx[i]).getMeanX(levelIdx, bestRadius);
		break;
            case 1:
                yData[i] = _simplexResults->at(volIdx[i]).getMeanY(levelIdx, bestRadius);
		break;
            case 2:
                yData[i] = _simplexResults->at(volIdx[i]).getRadius(bestRadius);
		break;
            case 3:
                yData[i] = _simplexResults->at(volIdx[i]).getMaxVT(levelIdx,bestRadius);
		break;
            }
        }
	
        // here xData yData is ready, we'll do the polynomial fitting
        // we iterate through all possible order and try to find a best order?
	
        float lastRSS;
        for(int nOrder = 1; nOrder < MAX_ORDER; ++nOrder) {
            float* coeff = new float[nOrder + 1];
            float  fitRSS;
            _polyFit(nOrder, nData, xData, yData, coeff, fitRSS);
	    
            if(nOrder > 1) {
                if(_fTest(lastRSS, nData - nOrder + 1, fitRSS, nData - nOrder)) {
                    lastRSS = fitRSS;
                    bestOrder[n] = nOrder;
                    bestRSS[n] = fitRSS / (nData - nOrder);
                    for(int k = 0; k < nOrder; k++)
                        bestCoeff[n][k] = coeff[k];
                }
                else
                    break;
            }
            else {
                lastRSS = fitRSS;
                bestOrder[n] = nOrder;
                bestRSS[n] = fitRSS / (nData - nOrder);
                for(int k = 0; k < nOrder; k++)
                    bestCoeff[n][k] = coeff[k];
            }
            delete[] coeff;
        }
    }
    //use the best fitting model to 'correct' the center
    
    int bestRadius = _ppBestRadius[volIdx.last()][levelIdx];
    float fitX, fitY, fitRad, fitWind;
    
    _polyCal(bestOrder[0],bestCoeff[0], _simplexResults->at(volIdx.last()).getMeanX(levelIdx, bestRadius), fitX);
    _polyCal(bestOrder[1],bestCoeff[1], _simplexResults->at(volIdx.last()).getMeanY(levelIdx, bestRadius), fitY);
    _polyCal(bestOrder[2],bestCoeff[2], _simplexResults->at(volIdx.last()).getRadius(bestRadius), fitRad);
    _polyCal(bestOrder[3],bestCoeff[3], _simplexResults->at(volIdx.last()).getMaxVT(levelIdx, bestRadius), fitWind);

    Center bestCenter;
    float minError =999.0f, error, xError, yError, radError, vtError;
    for(int ridx = 0; ridx < _simplexResults->last().getNumRadii(); ridx++) {
        for(int pidx = 0; pidx < _simplexResults->last().getNumPointsUsed(); pidx++) {
            Center tmpCenter = _simplexResults->last().getCenter(levelIdx, ridx, pidx);
            if(tmpCenter.isValid())
                continue;
            xError  = exp(-0.5f *pow((fitX - tmpCenter.getX()) / sqrt(bestRSS[0]), 2.0f)) * _paramPosWeight;
            yError  = exp(-0.5f *pow((fitY - tmpCenter.getY()) / sqrt(bestRSS[1]), 2.0f)) * _paramPosWeight;
            radError= exp(-0.5f *pow((fitRad - tmpCenter.getRadius()) / sqrt(bestRSS[2]), 2.0f)) * _paramRmwWeight;
            vtError = exp(-0.5f *pow((fitWind - tmpCenter.getMaxVT()) / sqrt(bestRSS[3]), 2.0f)) * _paramWindWeight;
            error = xError + yError + radError + vtError;
            if(error < minError){
                minError = error;
                bestCenter = tmpCenter;
            }
        }
    }

    // TODO This is broken. It puts the center outside of the grid.
    //                      because bestCenter.getX() and bestCenter.getY() are -999

    if ( (bestCenter.getX() == SimplexData::_fillv ) || (bestCenter.getY() == SimplexData::_fillv)) {
      std::cout << "ChooseCenter::_calPolyTest: bad bestCenter" << std::endl;
      retVal = false;
    } else {
      //modify the VortexData on this Level
      _vortexData->setLat(levelIdx, radarLat + bestCenter.getX() / fac_lat);
      _vortexData->setLon(levelIdx, radarLon + bestCenter.getY() / fac_lon);
      _vortexData->setHeight(levelIdx,_simplexResults->last().getHeight(levelIdx));
      _vortexData->setRMW(levelIdx, bestCenter.getRadius());
      _vortexData->setRMWUncertainty(levelIdx, radError);
      _vortexData->setCenterStdDev(levelIdx, sqrt(xError * xError + yError * yError));
      std::cout << "ChooseCenter found new poly center: "
		<< bestCenter.getX() << "," << bestCenter.getY()
		<< "," << bestCenter.getRadius() << std::endl;
    }
    
    //clear up
    
    delete[] xData;
    delete[] yData;
    delete[] bestOrder;
    delete[] bestRSS;
    for(int i = 0; i < 4; i++)
        delete[] bestCoeff[i];
    delete[] bestCoeff;
    return retVal;
}

bool ChooseCenter::_calPolyCenters()
{
    // This initial check relies on the idea that the last volume will be
    // the volume currently being processed, and additionally the most recent
    // volume in the time series.

    if((_simplexResults->last().getTime() > endTime) ||(_simplexResults->last().getTime() < startTime)) {
        std::cerr<<"Vortex is not within the specified time period for constructing polynomials."<<std::endl;
        return false;
    }

    this->findHeights();

    const int maxPolyArray = _simplexResults->count()>20?21:_simplexResults->count();

    // Construct a least squares polynomial to fit track

    _ppBestFitVariance = new float*[4];
    _ppBestFitDegree = new int*[4];
    _pppBestFitCoeff = new float**[4];
    for(int criteria = 0; criteria < 4; criteria++) {
        _ppBestFitVariance[criteria] = new float[numHeights->count()];
        _ppBestFitDegree[criteria] = new int[numHeights->count()];
        _pppBestFitCoeff[criteria] = new float*[numHeights->count()];
        for(int k = 0;k < numHeights->count(); k++){
            _ppBestFitVariance[criteria][k] = 0;
            _ppBestFitDegree[criteria][k] = 0;
            _pppBestFitCoeff[criteria][k] = new float[maxPolyArray];
            for(int b = 0; b < maxPolyArray; b++)
                _pppBestFitCoeff[criteria][k][b] = 0;
        }
    }

    QList<int> heights = numHeights->keys();
    // Sort heights so that k index is attached to a specific height
    // Sort smallest to largest
    // The sorting is nessecary because QHash randomizes ordering
    for(int s = 0; s < heights.count()-1; s++)
        for(int t = s+1; t < heights.count(); t++)
            if(heights[t] < heights[s])
                heights.swap(t,s);

    for(int k = 0; k < numHeights->count(); k++) {
        int currHeight = heights[k];
        int* levelIndices = indexOfHeights->value(currHeight);

        // In a lot of places we will replace k with
        // levelIndices[simplexResultIndex] to keep the height consistant

        int maxPoly = 0;

        // We want to find the eariliest time for this process;
        // we don't want to use (simplexList)sort this late in the game
        // because it might ruin the member arrays

        int timeRefIndex = 0;
        firstTime = QDateTime();
        while(levelIndices[timeRefIndex]==-1) {
            timeRefIndex++;
        }
        firstTime = _simplexResults->at(timeRefIndex).getTime();
        int goodVolumes = numHeights->value(currHeight);
        for(int i = timeRefIndex+1; i < _simplexResults->count(); i++) {
            if(levelIndices[i]!=-1) {
                if(_simplexResults->at(i).getTime()<firstTime) {
                    timeRefIndex = i;
                    firstTime = _simplexResults->at(i).getTime();
                }
            }
        }

        // What should the highest order basis polynomial;
        if(goodVolumes > 20)
            maxPoly = 20;
        else
            maxPoly = goodVolumes-1;

        if(maxPoly < 3){
            std::cerr<<"Cannot Adequately Fit Storm Parameters to Polynomials of Less Than 3rd Order"<<std::endl;
            continue;
        }

        // Things are stored here so far but never used
        // Store the variance of each polynomial fit by the maximum order
        // polynomial that the fit uses
        float* squareVariance = new float[maxPoly+1];

        // Initialize fitting matrices
        float* BB = new float[goodVolumes];
        float** MM = new float*[maxPoly+1];
        for(int rr = 0; rr <=maxPoly; rr++) {
            MM[rr] = new float[goodVolumes];
            for(int ii = 0; ii < goodVolumes; ii++) {
                MM[rr][ii] = 0;
                BB[ii] = 0;
            }
        }

        float *lastFitCoeff = new float[maxPoly+1];
        float *currentCoeff = new float[maxPoly+1];
        for(int ii = 0; ii <= maxPoly; ii++) {
            lastFitCoeff[ii] = 0;
            currentCoeff[ii] = 0;
        }

        for(int criteria = 0; criteria < 4; criteria++) {
            // Iterates through the procedure for the four different curve fitting criteria
            for(int ii = 0; ii <= maxPoly; ii++)
                squareVariance[ii] = 0;

            for(int n = 1; n <=maxPoly; n++) {
                for(int m = 0; m < n; m++) {
                    // Copy the last set into the lastFit array
                    lastFitCoeff[m]= currentCoeff[m];
                    currentCoeff[m] = 0;
                }
                currentCoeff[n] = 0;
                for(int ii = 0; ii < goodVolumes; ii++) {
                    BB[ii] = 0;
                    for(int rr = 0; rr <= maxPoly; rr++)
                        MM[rr][ii]= 0;
                }

                int gv = 0;
                for(int i = 0; i < _simplexResults->count(); i++) {
                    // Only take those values which are in specified time range and additionally have a level at this height
                    // Both of these characteristics are specified if the value of the index in indexOfHeights value array is not set to -1.  This is done in findHeights()
                    if(levelIndices[i]!=-1) {

                        for(int order = 0; order <= n; order++) {
                            float min = firstTime.secsTo(_simplexResults->at(i).getTime())/60.0;
                            MM[order][gv] = pow(min,(double)(order));
                        }
                        int jBest = _ppBestRadius[i][levelIndices[i]];
                        float y = 0;
                        switch(criteria) {
                        case 0:
                            y = _simplexResults->at(i).getMeanX(levelIndices[i], jBest); break;
                        case 1:
                            y = _simplexResults->at(i).getMeanY(levelIndices[i], jBest); break;
                        case 2:
                            y = _simplexResults->at(i).getRadius(jBest); break;
                        case 3:
                            y = _simplexResults->at(i).getMaxVT(levelIndices[i], jBest);
                            break;
                        }
                        BB[gv] = y;
                        gv++;
                    }
                }

                float stDev;
                float *stError = new float[n+1];
                if(!Matrix::lls(n+1,goodVolumes,MM,BB,stDev,currentCoeff,stError)) {
                    std::cerr<<"Least Squares Fit Failed in Construct Polynomial"<<std::endl;
                    return false;
                }
                delete [] stError;

                //use these coefficient to
                float errorSum = 0;
                for(int i = 0; i < _simplexResults->count(); i++) {
                    if(levelIndices[i]!=-1) {
                        float min = ((float)firstTime.secsTo(_simplexResults->at(i).getTime())/60.0);
                        float func_y  = 0;
                        for(int m = 0; m <=n; m++) {
                            func_y += currentCoeff[m]*pow(min,m);
                        }
                        //if(criteria == 2)
                        //Message::toScreen(" Fitted Radius = "+QString().setNum(func_y));
                        int jBest = _ppBestRadius[i][levelIndices[i]];
                        switch(criteria) {
                        case 0:
                            errorSum +=pow(func_y-_simplexResults->at(i).getMeanX(levelIndices[i], jBest), 2); break;
                        case 1:
                            errorSum +=pow(func_y-_simplexResults->at(i).getMeanY(levelIndices[i], jBest), 2); break;
                        case 2:
                            errorSum +=pow(func_y-_simplexResults->at(i).getRadius(jBest),2); break;
                        case 3:
                            errorSum +=pow(func_y-_simplexResults->at(i).getMaxVT(levelIndices[i], jBest), 2);
                            break;
                        }
                    }
                }
                int degFreedom = goodVolumes-n;
                if(degFreedom == 0)
                    Message::toScreen("ChooseCenter construct Polynomial, division by zero");
                squareVariance[n] = errorSum/(float)degFreedom;

                if(n > 1) {
                    if(squareVariance[n] > squareVariance[n-1]) {
                        // we found the best fit!!!!
                        _ppBestFitVariance[criteria][k] = squareVariance[n-1];
                        _ppBestFitDegree[criteria][k] = n-1;
                        // Now we have to revert back to the old set
                        // This is why everything is kept in storage
                        for(int l = 0; l <= n; l++) {
                            _pppBestFitCoeff[criteria][k][l] = lastFitCoeff[l];
                        }
                        break;
                    }
                    else {
                        if(n == maxPoly) {
                            // Maxed out
                            _ppBestFitVariance[criteria][k] = squareVariance[n];
                            _ppBestFitDegree[criteria][k] = n;
                            for(int l = 0; l <= n; l++) {
                                _pppBestFitCoeff[criteria][k][l] = currentCoeff[l];
                            }
                        }
                        else {
                            //Message::toScreen("Using F-Test");
                            // preform the f test to see if a higher polynomial is warranted
                            float fTest;
                            float m_delta = squareVariance[n-1]*(float)(degFreedom+1)-squareVariance[n]*(float)degFreedom;
                            if(squareVariance[n] > 0)
                                fTest = m_delta/squareVariance[n];
                            else
                                fTest = 0;
                            if(degFreedom >30)
                                degFreedom = 30;
                            float fCrit = _fCriteria[degFreedom-1];
                            if(fTest < fCrit) {
                                // Found the best center
                                _ppBestFitVariance[criteria][k] = squareVariance[n-1];
                                _ppBestFitDegree[criteria][k] = n-1;
                                // degree = n-1; phasing out
                                for(int l = 0; l <= (n-1); l++) {
                                    _pppBestFitCoeff[criteria][k][l] = lastFitCoeff[l];
                                }
                            }
                        }
                    }
                }
                else {
                    // Doing a linear fit with only 3 points
                    _ppBestFitVariance[criteria][k] = squareVariance[n];
                    _ppBestFitDegree[criteria][k] = n;
                    for(int l = 0; l <= n; l++) {
                        _pppBestFitCoeff[criteria][k][l] = currentCoeff[l];
                    }
                }
            }
        }
        //Message::toScreen("Got to deleting arrays");
        for(int ii = 0; ii <= maxPoly; ii++) {
            delete [] MM[ii];
        }
        delete [] MM;
        delete [] BB;
        delete [] squareVariance;
        delete [] lastFitCoeff;
        delete [] currentCoeff;
        //delete [] stError;
    }

    return true;
}



bool ChooseCenter::fixCenters()
{

    // Set up parameters for populating the vortexData
    float radarLat = _config->getParam(_config->getConfig("radar"), QString("lat")).toFloat();
    float radarLon = _config->getParam(_config->getConfig("radar"), QString("lon")).toFloat();
    float radarLatRadians = radarLat * acos(-1.0)/180.0;
    float fac_lat = 111.13209 - 0.56605 * cos(2.0 * radarLatRadians) + 0.00012 * cos(4.0 * radarLatRadians) - 0.000002 * cos(6.0 * radarLatRadians);
    float fac_lon = 111.41513 * cos(radarLatRadians) - 0.09455 * cos(3.0 * radarLatRadians) + 0.00012 * cos(5.0 * radarLatRadians);


    _ppNewBestRadius = new int*[_simplexResults->count()];
    _ppNewBestCenter = new int*[_simplexResults->count()];
    for(int i = 0; i < _simplexResults->count(); i++) {
        _ppNewBestRadius[i] = new int[numHeights->count()];
        _ppNewBestCenter[i] = new int[numHeights->count()];
        for(int k = 0; k < numHeights->count(); k++) {
            _ppNewBestRadius[i][k] = 0;
            _ppNewBestCenter[i][k] = 0;
        }
    }

    // Get the volume with the latest time so we know which one we are
    // currently working on.

    // set firstTime to the earliest volume in the simplexResults set

    firstTime = _simplexResults->at(0).getTime();
    for(int ii = 1; ii < _simplexResults->count(); ii++) {
        if(_simplexResults->at(ii).getTime() < firstTime)
            firstTime = _simplexResults->at(ii).getTime();
    }

    // Find the last index for picking a center;
    int lastTimeIndex = 0;
    float longestTime = 0;
    for(int i = 0; i < _simplexResults->count(); i++) {
        if(firstTime.secsTo(_simplexResults->at(i).getTime())/60.f > longestTime) {
            lastTimeIndex = i;
            longestTime = firstTime.secsTo(_simplexResults->at(i).getTime())/60.f;
        }
    }

    QList<int> heights = numHeights->keys();
    // Sort heights so that k index is attached to a specific height
    // Sort smallest to largest
    // The sorting is nessecary because QHash randomizes ordering
    for(int s = 0; s < heights.count()-1; s++)
        for(int t = s+1; t < heights.count(); t++)
            if(heights[t] < heights[s])
                heights.swap(t,s);

    for(int hidx = 0; hidx < numHeights->count(); hidx++) {

        int currHeight = heights[hidx];  // in meters
        // int goodVolumes = numHeights->value(currHeight);
        int* levelIndices = indexOfHeights->value(currHeight);

        if(levelIndices[lastTimeIndex]==-1){
            continue;
        }
        int timeRefIndex = 0;
        firstTime = QDateTime();
        while(levelIndices[timeRefIndex]==-1) {
            timeRefIndex++;
        }
        firstTime = _simplexResults->at(timeRefIndex).getTime();
        for(int i = timeRefIndex+1; i < _simplexResults->count(); i++) {
            if(levelIndices[i]!=-1) {
                if(_simplexResults->at(i).getTime()<firstTime) {
                    timeRefIndex = i;
                    firstTime = _simplexResults->at(i).getTime();
                }
            }
        }

        // Do work that fixCenters is supposed to

        for(int vidx = 0; vidx < _simplexResults->count(); vidx++) {
            if(levelIndices[vidx]!=-1){
                float polyFitx = 0;
                float polyFity = 0;
                float polyFitrad = 0;
                float polyFitwind = 0;
                // float finalStd = 0;
                // float confidence = 0;
                // float stdError = 0;
                float min = ((float)firstTime.secsTo(_simplexResults->at(vidx).getTime())/60.0);
                //check out use of n up to best degree of fit based on assignment
                for(int n = 0; n <= _ppBestFitDegree[0][hidx]; n++) {
                    polyFitx += _pppBestFitCoeff[0][hidx][n]*pow(min,n);
                }
                for(int n = 0; n <= _ppBestFitDegree[1][hidx]; n++) {
                    polyFity += _pppBestFitCoeff[1][hidx][n]*pow(min,n);
                }
                for(int n = 0; n <= _ppBestFitDegree[2][hidx]; n++) {
                    polyFitrad += _pppBestFitCoeff[2][hidx][n]*pow(min,n);
                }
                for(int n = 0; n <= _ppBestFitDegree[3][hidx]; n++) {
                    polyFitwind += _pppBestFitCoeff[3][hidx][n]*pow(min,n);
                }

                float totalMinError = 0;
                float totalError = 0;

                for(int ridx = 0; ridx < _simplexResults->at(vidx).getNumRadii(); ridx++) {
                    float xError, yError, radError, windError;
                    for(int pidx = 0; pidx < _simplexResults->at(vidx).getNumPointsUsed(); pidx++) {
                        if(!_simplexResults->at(vidx).getCenter(levelIndices[vidx],ridx,pidx).isValid()) {

                            Center currCenter = _simplexResults->at(vidx).getCenter(levelIndices[vidx],ridx,pidx);
                            float stdX = sqrt(_ppBestFitVariance[0][hidx]);
                            xError = exp(-.5*pow((polyFitx-currCenter.getX())/stdX, 2));
                            float stdY = sqrt(_ppBestFitVariance[1][hidx]);
                            yError = exp(-.5*pow((polyFity-currCenter.getY())/stdY, 2));
                            float stdRad = sqrt(_ppBestFitVariance[2][hidx]);
                            if(stdRad == 0) {
                                radError = 1;
                            }
                            else {
                                radError = exp(-.5*pow((polyFitrad-currCenter.getRadius())/stdRad, 2));
                            }
                            float stdWind = sqrt(_ppBestFitVariance[3][hidx]);
                            windError = exp(-.5*pow((polyFitwind-currCenter.getMaxVT())/stdWind, 2));
                            xError    *= _paramPosWeight;
                            yError    *= _paramPosWeight;
                            radError  *= _paramRmwWeight;
                            windError *= _paramVelWeight;
                            totalError = xError+yError+radError+windError;

                            if(totalError > totalMinError) {
                                totalMinError = totalError;
                                _ppNewBestRadius[vidx][hidx] = ridx;
                                _ppNewBestCenter[vidx][hidx] = pidx;
                            }
                        }
                    }
                }
                // new variables here but why?

                // get best simplex center;
                Center bestCenter = _simplexResults->at(vidx).getCenter(levelIndices[vidx],_ppNewBestRadius[vidx][hidx],_ppNewBestCenter[vidx][hidx]);
                float xError, yError, radError;

                xError = (polyFitx-bestCenter.getX())*(polyFitx-bestCenter.getX());
                yError = (polyFity-bestCenter.getY())*(polyFity-bestCenter.getY());
                radError = (polyFitrad-bestCenter.getRadius())*(polyFitrad-bestCenter.getRadius());

                if(vidx == lastTimeIndex) {
                    // if the volume we are looking at is the last one we will want to keep all the info in vortexData
                    // We don't check to see that any of these levels are in the search zone for vortexData in vtd

                    // int j = _ppBestRadius[vidx][levelIndices[vidx]];

                    float centerLat = radarLat + bestCenter.getY()/fac_lat;
                    float centerLon = radarLon + bestCenter.getX()/fac_lon;
                    _vortexData->setLat(levelIndices[vidx], centerLat);
                    _vortexData->setLon(levelIndices[vidx], centerLon);
                    _vortexData->setHeight(levelIndices[vidx], _simplexResults->at(vidx).getHeight(levelIndices[vidx]));
                    _vortexData->setMaxVT(levelIndices[vidx],bestCenter.getMaxVT());
                    _vortexData->setRMW(levelIndices[vidx], bestCenter.getRadius());
                    _vortexData->setRMWUncertainty(levelIndices[vidx], radError);
                    _vortexData->setCenterStdDev(levelIndices[vidx], sqrt(xError*xError+yError*yError));
                }
            }
        }

        // the newVariance is never used in the perl code of choose center,
        // and the variable degree is never initialized either, it is just left at
        // the last degree checked for the last function, probably wind
        // is this intentional or not??
        // my best guess is that degree will be set to the high degree for wind
        // so that is what I will set it to here
    }
    return true;
}

void ChooseCenter::_useLastMean()
{
    // Fake fill of vortexData for testing purposes
  
    float radarLat = _config->getParam(_config->getConfig("radar"), QString("lat")).toFloat();
    float radarLon = _config->getParam(_config->getConfig("radar"), QString("lon")).toFloat();
    float radarLatRadians = radarLat * acos(-1.0) / 180.0;
    float fac_lat = 111.13209 - 0.56605 * cos(2.0 * radarLatRadians)
      + 0.00012 * cos(4.0 * radarLatRadians) - 0.000002 * cos(6.0 * radarLatRadians);
    float fac_lon = 111.41513 * cos(radarLatRadians) - 0.09455 * cos(3.0 * radarLatRadians)
      + 0.00012 * cos(5.0 * radarLatRadians);

    for(int k = 0; k < _simplexResults->last().getNumLevels(); k++) {
        int bestRadii = _ppBestRadius[_simplexResults->size() - 1][k];
	
	// TODO _simplexResults->last().getMeanY(k,bestRadii) could be -999
	// Seems to happen when bestRadii is 0, but this is probably just one case.
	// When that is the case, the vortexData Lat and Lon is bogus.

	// What do we do. Maybe set a very bad stdDev so it won't be selected in VoetexThread::run() ?
	// what about the other fields?
	
	float meanX = _simplexResults->last().getMeanX(k, bestRadii);
	float meanY = _simplexResults->last().getMeanY(k, bestRadii);
	float centerLat = radarLat + meanY / fac_lat;
        float centerLon = radarLon + meanX / fac_lon;

        _vortexData->setLat(k, centerLat);
        _vortexData->setLon(k, centerLon);
        _vortexData->setHeight(k, _simplexResults->last().getHeight(k));
        _vortexData->setMaxVT(k, _simplexResults->last().getMaxVT(k, bestRadii));
        _vortexData->setRMW(k, _simplexResults->last().getRadius(bestRadii));
        _vortexData->setRMWUncertainty(k, SimplexData::_fillv);
	if( (meanX == SimplexData::_fillv) || (meanY == SimplexData::_fillv) )
	  _vortexData->setCenterStdDev(k, std::abs((int) SimplexData::_fillv));
	else
	  _vortexData->setCenterStdDev(k, _simplexResults->last().getCenterStdDev(k, bestRadii));
    }
}

void ChooseCenter::findHeights()
{
    numHeights = new QHash<int, int>;
    indexOfHeights = new QHash<int,int*>;

    for(int i = 0; i < _simplexResults->count(); i++) {
        if((_simplexResults->at(i).getTime() >= startTime) &&(_simplexResults->at(i).getTime() <= endTime)) {
            for(int j = 0; j < _simplexResults->at(i).getNumLevels(); j++) {
                int currHeight = int(_simplexResults->at(i).getHeight(j)*1000+.5);
                if(numHeights->value(currHeight,-1)==-1) {
                    numHeights->insert(currHeight,1);
                    int* newHeightArray = new int[_simplexResults->count()];
                    for(int kk = 0; kk < _simplexResults->count(); kk++)
                        newHeightArray[kk] = -1;
                    indexOfHeights->insert(currHeight,newHeightArray);
                }
                else
                    numHeights->insert(currHeight, numHeights->value(currHeight)+1);
            }
        }
    }

    for(int i = 0; i < _simplexResults->count(); i++) {
        if((_simplexResults->at(i).getTime() >= startTime) && (_simplexResults->at(i).getTime() <= endTime)) {
            for(int j = 0; j < _simplexResults->at(i).getNumLevels(); j++) {
                int currHeight = int(_simplexResults->at(i).getHeight(j)*1000+.5);
                int* heightProfile = indexOfHeights->take(currHeight);
                heightProfile[i] = j;
                indexOfHeights->insert(currHeight,heightProfile);
            }
        }
    }

    QHash<int, int>::const_iterator i1 = numHeights->constBegin();
    while (i1 != numHeights->constEnd()) {
        std::cout << i1.key() << ": " << i1.value() << std::endl;
        ++i1;
    }
    QHash<int, int*>::const_iterator i = indexOfHeights->constBegin();
    while(i!=indexOfHeights->constEnd()){
        std::cout << i.key() << ": "<< i.value() << std::endl;
        ++i;
    }

    QList<int> heights = numHeights->keys();
    for(int i = 0; i < heights.count(); i++) {
        if(numHeights->value(heights[i]) <= 3) {
            numHeights->remove(heights[i]);
            indexOfHeights->remove(heights[i]);
            continue;
        }
    }

}

bool ChooseCenter::_polyFit(const int nCoeff, const int nData, const float* xData, const float* yData, float* aData, float& rss )
{
    float** A=new float*[nCoeff+1];
    for(int i=0;i<=nCoeff;i++)
        A[i]=new float[nData];
    for(int i=0;i<=nCoeff;i++)
        for(int j=0;j<nData;j++)
            A[i][j]=pow(xData[j],float(i));
    float* b=new float[nData];
    for(int i=0;i<nData;i++)
        b[i]=yData[i];
    float* stError=new float[nData];
    float dummyStd;
    Matrix::lls(nCoeff+1,nData,A,b,dummyStd,aData,stError);
    for(int i=0;i<nData;i++)
        _polyCal(nCoeff,aData,xData[i],b[i]);
    rss =0.0f;
    for(int i=0;i<nData;i++)
        rss +=(b[i]-yData[i])*(b[i]-yData[i]);

    delete[] stError;
    delete[] b;
    for(int i=0;i<=nCoeff;i++)
        delete[] A[i];
    delete[] A;
	return true;
}

void ChooseCenter::_polyCal(const int nOrder, const float* aData, const float xData, float& yData)
{
    /* Notice: here nOrder is the order of polynomial fitting, aData should be a rray of length nOrder+1
     */

    yData =0.f;
    for(int i=0;i<=nOrder;i++)
        yData +=aData[i]*pow(xData,float(i));
}

void ChooseCenter::_polyTest()
{

    std::srand((unsigned)std::time(0));
    const int nOrder =10;
    const int nData  =100;

    float  xData[nData],yData[nData],a[nOrder],b[nOrder];
    for(int i=0;i<nOrder;i++)
        a[i]=(std::rand()%10)/10.f;

    for(int i=0;i<nData;i++){
        xData[i]=(std::rand()%10)/10.f;
        yData[i]=0.f;
        for(int j=0;j<nOrder;j++)
            yData[i]+=a[j]*pow(xData[i],float(j));
        //yData[i]+=(std::rand()%10)/50.f;
    }
    float fitStd;
    _polyFit(nOrder,nData,xData,yData,b,fitStd);
    std::cout<<"Fitting Result:"<<std::endl;
    for(int i=0;i<nOrder;i++)
        std::cout<<a[i]<<"---"<<b[i]<<std::endl;
    std::cout<<"Fit std: "<<fitStd<<std::endl;
}

bool ChooseCenter::_fTest(const float& RSS1,const int& freedom1,const float& RSS2,const int& freedom2)
{
    float fTest =((RSS1-RSS2)/(freedom1-freedom2))/(RSS2/freedom2);
    return freedom1>29?fTest>_fCriteria[29]:fTest>_fCriteria[freedom1];
}
