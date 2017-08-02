/*
 *  SimplexData.cpp
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 5/30/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "SimplexData.h"
#include "Message.h"
#include <QTextStream>

constexpr float SimplexData::_fillv;

SimplexData::SimplexData()
{
    numLevels = MAXLEVELS;
    numRadii = MAXRADII;
    numCenters = MAXCENTERS;
    numPointsUsed = 0;

    for(int i = 0; i < numLevels; i++)
    {
        //height[i] = velNull;
        for(int j = 0; j < numRadii; j++) {
            meanX[i][j] = _fillv;
            meanY[i][j] = _fillv;
            centerStdDeviation[i][j] = _fillv;
            meanVT[i][j] = _fillv;
            meanVTUncertainty[i][j] = _fillv;
            numConvergingCenters[i][j] = (int)_fillv;
            radius[j] = _fillv;
            for(int k = 0; k < numCenters; k++) {
                centers[i][j][k] = Center();
                initialX[i][j][k] = _fillv;
                initialY[i][j][k] = _fillv;
            }
        }
    }

    time = QDateTime();

}

SimplexData::SimplexData(int availLevels, int availRadii, int availCenters)
{
  numLevels = availLevels;        // TODO numLevels could be larger than MAXLEVELS
  numRadii = availRadii;
  numCenters = availCenters;
  numPointsUsed = 0;

  for(int i = 0; i < numLevels; i++)
    {
      height[i] = _fillv;
      for(int j = 0; j < numRadii; j++)
        {
	  meanX[i][j] = _fillv;
	  meanY[i][j] = _fillv;
	  centerStdDeviation[i][j] = _fillv;
	  meanVT[i][j] = _fillv;
	  meanVTUncertainty[i][j] = _fillv;
	  numConvergingCenters[i][j]= int(_fillv);
	  radius[j] = _fillv;
	  for(int k = 0; k < numCenters; k++) {
	    centers[i][j][k] = Center();
	    initialX[i][j][k] = _fillv;
	    initialY[i][j][k] = _fillv;
	  }
        }
    }

  time = QDateTime();
}

SimplexData::SimplexData(const SimplexData& other)
{
    this->numLevels = other.numLevels;
    this->numRadii = other.numRadii;
    this->numCenters = other.numCenters;
    this->numPointsUsed = other.numPointsUsed;

    for(int i = 0; i < this->numLevels; i++)
    {
        this->height[i] = other.height[i];
        for(int j = 0; j < this->numRadii; j++)
        {
            this->meanX[i][j] = other.meanX[i][j];
            this->meanY[i][j] = other.meanY[i][j];
            this->centerStdDeviation[i][j] = other.centerStdDeviation[i][j];
            this->meanVT[i][j] = other.meanVT[i][j];
            this->meanVTUncertainty[i][j] = other.meanVTUncertainty[i][j];
            this->numConvergingCenters[i][j]= other.numConvergingCenters[i][j];
            this->radius[j] = other.radius[j];
            for(int k = 0; k < this->numCenters; k++) {
                this->centers[i][j][k] = other.centers[i][j][k];
                initialX[i][j][k] = other.initialX[i][j][k];
                initialY[i][j][k] = other.initialY[i][j][k];
            }
        }
    }

    this->time = other.time;
}

SimplexData::~SimplexData()
{
}

float SimplexData::getMeanX(const int& lev, const int& rad) const
{
    if ((lev < numLevels)&&(rad<numRadii))
        return meanX[lev][rad];
    Message::toScreen("SimplexData: getX: Outside Bounds");
    return meanX[0][0];
}

void SimplexData::setMeanX(const int& lev, const int& rad, const float& newX)
{
    if ((lev < numLevels)&&(rad < numRadii))
        meanX[lev][rad] = newX;
    else
        Message::toScreen("SimplexData: setX: Outside Bounds");
}

void SimplexData::setMeanX(const float** a, const int& numLev, const int& numRad)
{
    for (int i = 0; i < numLev; i++)
        for(int j = 0; j < numRad; j++)
            meanX[i][j] = a[i][j];
}

float SimplexData::getMeanY(const int& lev, const int& rad) const
{
    if ((lev < numLevels) && (rad < numRadii))
        return meanY[lev][rad];
    Message::toScreen("SimplexData: getY: Outside Bounds: Level = "+QString().setNum(lev) +
		      " Radius = " + QString().setNum(rad));
    return meanY[0][0];
}

void SimplexData::setMeanY(const int& lev, const int& rad, const float& newY)
{
    if ((lev < numLevels)&&(rad < numRadii))
        meanY[lev][rad] = newY;
    else
        Message::toScreen("SimplexData: setX: Outside Bounds");
}

void SimplexData::setMeanY(const float** a, const int& numLev, const int& numRad)
{
    for (int i = 0; i < numLev; i++)
        for(int j = 0; j < numRad; j++)
            meanY[i][j] = a[i][j];
}

float SimplexData::getCenterStdDev(const int& lev, const int& rad) const
{
    if((lev < numLevels)&&(rad < numRadii))
        return centerStdDeviation[lev][rad];
    Message::toScreen("SimplexData: getCenterStdDev: Outside Bounds");
    return centerStdDeviation[0][0];
}

void SimplexData::setCenterStdDev(const int& lev, const int& rad,
                                  const float& number)
{
    if((lev < numLevels)&&(rad < numRadii))
        centerStdDeviation[lev][rad] = number;
    else
        Message::toScreen("SimplexData: setCenterStdDev: Outside Bounds");
}

void SimplexData::setCenterStdDev(const float** a, const int& numLev, 
                                  const int& numRad)
{
    for(int i = 0; i < numLev; i++)
        for(int j = 0; j < numRad; j++)
            centerStdDeviation[i][j]=a[i][j];
}

float SimplexData::getHeight(const int& i) const
{
    if (i < numLevels)
        return height[i];
    Message::toScreen("SimplexData: getHeight: Outside Bounds");
    return height[0];
}


void SimplexData::setHeight(const int& index, const float& newHeight)
{
    if (index < numLevels)
        height[index] = newHeight;
    else
        Message::toScreen("SimplexData: setHeight: Outside Bounds");
}

void SimplexData::setHeight(const float* a, const int& numLev)
{
    for (int i = 0; i < numLev; i++)
        height[i] = a[i];
}

float SimplexData::getRadius(const int& i) const
{
    if (i < numRadii)
        return radius[i];
    Message::toScreen("SimplexData: getRadius: Outside Bounds");
    return radius[0];
}


void SimplexData::setRadius(const int& index, const float& newRadius)
{
    if (index < numRadii)
        radius[index] = newRadius;
    else
        Message::toScreen("SimplexData: setRadius: Outside Bounds");
}

void SimplexData::setRadius(const float* a, const int& numRad)
{
    for (int i = 0; i < numRad; i++)
        radius[i] = a[i];
}

QDateTime SimplexData::getTime() const
{
    return time;
}

void SimplexData::setTime(const QDateTime& radarTime)
{
    time = QDateTime(radarTime);
}

float SimplexData::getMaxVT(const int& lev, const int& rad) const
{
    if ((lev < numLevels)&&(rad < numRadii))
        return meanVT[lev][rad];
    Message::toScreen("SimplexData: getMaxVT: Outside Bounds");
    return meanVT[0][0];
}

void SimplexData::setMaxVT(const int& lev, const int& rad, const float& vel)
{
    if ((lev < numLevels)&&(rad < numRadii))
        meanVT[lev][rad] = vel;
    else
        Message::toScreen("SimplexData: setMaxVT: Outside Bounds");
}

void SimplexData::setMaxVT(const float** a, const int& numLev, 
                           const int& numRad)
{
    for ( int i = 0; i < numLev; i++)
        for(int j = 0; j < numRad; j++)
            meanVT[i][j] = a[i][j];
}

float SimplexData::getVTUncertainty(const int& lev, const int& rad) const
{
    if((lev < numLevels)&&(rad < numRadii))
        return meanVTUncertainty[lev][rad];
    Message::toScreen("SimplexData: getVTUncertainty: Outside Bounds");
    return meanVTUncertainty[0][0];
}

void SimplexData::setVTUncertainty(const int& lev, const int& rad, 
                                   const float& dMaxVT)
{
    if ((lev < numLevels)&&(rad < numRadii))
        meanVTUncertainty[lev][rad] = dMaxVT;
    else
        Message::toScreen("SimplexData: setVTUncertainty: Outside Bounds");
}

void SimplexData::setVTUncertainty(const float** a, const int& numLev, 
                                   const int& numRad)
{
    for (int i = 0; i < numLev; i++)
        for (int j = 0; j < numRad; j++)
            meanVTUncertainty[i][j] = a[i][j];
}

int SimplexData::getNumConvergingCenters(const int& lev, const int& rad) const
{
    if((lev < numLevels) && (rad < numRadii))
        return numConvergingCenters[lev][rad];
    Message::toScreen("SimplexData: getNumConvergingCenters: Outside Bounds Level = " + QString().setNum(lev)
		      + " radius = " + QString().setNum(rad));
    return numConvergingCenters[0][0];
}

void SimplexData::setNumConvergingCenters(const int& lev, const int& rad, 
                                          const int& num)
{
    if((lev < numLevels)&&(rad < numRadii)&&(num < numPointsUsed)) {
        numConvergingCenters[lev][rad] = num;
        return;
    }

    Message::toScreen("SimplexData: setNumConvergingCenters: Failed to set - outside of ranges");
    return;

}

void SimplexData::setNumConvergingCenters(const int** a, const int& numLev, 
                                          const int& numRad)
{
    for (int i = 0; i < numLev; i++)
        for (int j = 0; j < numRad; j++)
            numConvergingCenters[i][j] = a[i][j];
}

Center SimplexData::getCenter(const int& lev, const int& rad, 
                              const int& waveNum) const
{
    if((lev < numLevels)&&(rad < numRadii)&&(waveNum < numCenters))
        return centers[lev][rad][waveNum];
    Message::toScreen("SimplexData: getCenter: Outside Bounds");
    return Center();
}

void SimplexData::setCenter(const int& lev, const int& rad, 
                            const int& waveNum, const Center &newCenter)
{
    centers[lev][rad][waveNum] = newCenter;
}

int SimplexData::getNumPointsUsed() const
{
    return numPointsUsed;
}

void SimplexData::setNumPointsUsed(const int& i)
{
    numPointsUsed = i;
}

bool SimplexData::operator ==(const SimplexData &other)
{
    if(this->time == other.time)
        return true;
    return false;
}

bool SimplexData::operator < (const SimplexData &other)
{
    if(this->time < other.time)
        return true;
    return false;
}

bool SimplexData::operator > (const SimplexData &other)
{
    if(this->time > other.time)
        return true;
    return false;
}

bool SimplexData::isNull()
{
    if(time.isNull()) {
        if(meanX[0][0] == _fillv)
            if(meanY[0][0] == _fillv)
                if(height[0]== _fillv)
                    if(radius[0] == _fillv)
                        if(meanVT[0][0] == _fillv)
                            if(numConvergingCenters[0][0]==(int)_fillv)
                                if(centers[0][0][0].isValid())
                                    return true;
    }
    return false;
}

bool SimplexData::emptyLevelRadius(const int& l, const int& r) const 
{
    if((l < numLevels)&&(r < numRadii)){
        if((meanX[l][r]==_fillv)||(meanY[l][r]==_fillv)
                ||(centerStdDeviation[l][r]==_fillv)
                ||(numConvergingCenters[l][r]==(int)_fillv)||(meanVT[l][r]==_fillv)
                ||(meanVTUncertainty[l][r]==_fillv))
            return true;
        return false;
    }
    return true;
}

void SimplexData::printString()
{
    QTextStream out(stdout);
    out<< endl;
    out<< "Printing SimplexData Time = "+getTime().toString(Qt::ISODate)<< endl;
    out<< "  time: "+getTime().toString(Qt::ISODate) << endl;
    for(int i = 0; i < 2; i++) {
        QString ii = QString().setNum(i);
        out<<"  meanX @ level:"+ii+": "+QString().setNum(getMeanX(i,0)) <<endl;
        out<<"  meanY @ level:"+ii+": "+QString().setNum(getMeanY(i,0)) << endl;
        out<<"  height @ level:"+ii+": "+QString().setNum(getHeight(i)) << endl;
        out<<"  first Radius: "+QString().setNum(getRadius(0)) << endl;
        out<<"  centerStdDev @ level:"+ii+": ";
        out<<         QString().setNum(getCenterStdDev(i,0)) << endl;
        out<<"  maxVT @ level:"+ii+": ";
        out<<         QString().setNum(getMaxVT(i,0)) << endl;
        out<<"  NumConvCenters @ level:"+ii+": ";
        out<<         QString().setNum(getNumConvergingCenters(i,0)) << endl;
        out<<"  MaxVtStdDev @ level:"+ii+": ";
        out<<         QString().setNum(getVTUncertainty(i,0)) << endl;
        out<<"  centers @ level:"+ii+": ";
        out<<         QString().setNum(getCenter(i,0,0).getMaxVT()) << endl;
        out<<"  ----------------------------------------------------------" <<endl;
    }
}

void SimplexData::setNumLevels(int newNumLevels)
{
    this->numLevels = newNumLevels;
}

void SimplexData::setNumRadii(int newNumRadii)
{
    this->numRadii = newNumRadii;
}

void SimplexData::setNumCenters(int newNumCenters) 
{
    this->numCenters = newNumCenters;
}

float SimplexData::getInitialX(const int& level, const int& rad, 
                               const int& center) const
{
    return initialX[level][rad][center];
}

float SimplexData::getInitialY(const int& level, const int& rad, 
                               const int& center) const
{
    return initialY[level][rad][center];
}

void SimplexData::setInitialX( int& level,  int& rad, 
                               int& center,  float& value)
{
    initialX[level][rad][center] = value;
}

void SimplexData::setInitialY( int& level,  int& rad, 
                               int& center,  float& value)
{
    initialY[level][rad][center] = value;
}
