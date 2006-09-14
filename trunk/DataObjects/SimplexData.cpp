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

SimplexData::SimplexData()
{
  numLevels = maxLevels;
  numRadii = maxRadii;
  numCenters = maxCenters;
  velNull = -999.; 
  for(int i = 0; i < numLevels; i++)
    {
      height[i] = velNull;
      for(int j = 0; j < numRadii; j++) {
	meanX[i][j] = velNull;
	meanY[i][j] = velNull;
	centerStdDeviation[i][j] = velNull;
	meanVT[i][j] = velNull;
	meanVTUncertainty[i][j] = velNull;
	numConvergingCenters[i][j] = (int)velNull;
	radius[j] = velNull;
	for(int k = 0; k < numCenters; k++) {
	  centers[i][j][k] = Center();
	}
      }
    }

  time = QDateTime();

}

SimplexData::SimplexData(int availLevels, int availRadii, int availCenters)
{
  numLevels = availLevels;
  numRadii = availRadii;
  numCenters = availCenters;
  float velNull = -999.;
  
  for(int i = 0; i < numLevels; i++)
    {
      height[i] = velNull;
      for(int j = 0; j < numRadii; j++) 
	{
	  meanX[i][j] = velNull;
	  meanY[i][j] = velNull;
	  centerStdDeviation[i][j] = velNull;
	  meanVT[i][j] = velNull;
	  meanVTUncertainty[i][j] = velNull;
	  numConvergingCenters[i][j]= int(velNull);
	  radius[j] = velNull;
	  for(int k = 0; k < numCenters; k++) {
	    centers[i][j][k] = Center();
	  }
	}
    }
  
  time = QDateTime();
   
}


SimplexData::~SimplexData()
{
}

float SimplexData::getX(const int& lev, const int& rad) const
{
  if ((lev < numLevels)&&(rad<numRadii))
    return meanX[lev][rad];
  return meanX[0][0];
}

void SimplexData::setX(const int& lev, const int& rad, const float& newX)
{
  if ((lev < numLevels)&&(rad < numRadii))
    meanX[lev][rad] = newX;
}

void SimplexData::setX(const float** a, const int& numLev, const int& numRad)
{
  for (int i = 0; i < numLev; i++)
    for(int j = 0; j < numRad; j++)
      meanX[i][j] = a[i][j];
}

float SimplexData::getY(const int& lev, const int& rad) const
{
  if ((lev < numLevels)&&(rad < numRadii))
    return meanY[lev][rad];
  return meanY[0][0];
}

void SimplexData::setY(const int& lev, const int& rad, const float& newY)
{
  if ((lev < numLevels)&&(rad < numRadii))
    meanY[lev][rad] = newY;
}

void SimplexData::setY(const float** a, const int& numLev, const int& numRad)
{
  for (int i = 0; i < numLev; i++)
    for(int j = 0; j < numRad; j++)
      meanY[i][j] = a[i][j];
}

float SimplexData::getCenterStdDev(const int& lev, const int& rad) const
{
  if((lev < numLevels)&&(rad < numRadii))
    return centerStdDeviation[lev][rad];
  return centerStdDeviation[0][0];
}

void SimplexData::setCenterStdDev(const int& lev, const int& rad,
				  const float& number)
{
  if((lev < numLevels)&&(rad < numRadii))
    centerStdDeviation[lev][rad] = number;
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
  return height[0];
}


void SimplexData::setHeight(const int& index, const float& newHeight)
{
  if (index < numLevels)
    height[index] = newHeight;
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
  return radius[0];
}


void SimplexData::setRadius(const int& index, const float& newRadius)
{
  if (index < numRadii)
    radius[index] = newRadius;
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
  return meanVT[0][0];
}

void SimplexData::setMaxVT(const int& lev, const int& rad, const float& vel)
{
  if ((lev < numLevels)&&(rad < numRadii))
    meanVT[lev][rad] = vel;
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
  return meanVTUncertainty[0][0];
}

void SimplexData::setVTUncertainty(const int& lev, const int& rad, 
				      const float& dMaxVT)
{
  if ((lev < numLevels)&&(rad < numRadii))
    meanVTUncertainty[lev][rad] = dMaxVT;
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
  if((lev < numLevels)&&(rad < numRadii))
    return numConvergingCenters[lev][rad];
  return numConvergingCenters[0][0];
}

void SimplexData::setNumConvergingCenters(const int& lev, const int& rad, 
					  const int& num)
{
 if((lev < numLevels)&&(rad < numRadii))
   numConvergingCenters[lev][rad] = num;
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
    if(meanX[0][0] == velNull)
      if(meanY[0][0] == velNull)
	if(height[0]== velNull)
	  if(radius[0] == velNull)
	    if(meanVT[0][0] == velNull)
	      if(numConvergingCenters[0][0]==(int)velNull)
		if(centers[0][0][0].isNull())
		  return true;
  }
  return false;
}

bool SimplexData::emptyLevelRadius(const int& l, const int& r) const 
{
  if((l < numLevels)&&(r < numRadii)){
    if((meanX[l][r]!=velNull)||(meanY[l][r]!=velNull)
       ||(centerStdDeviation[l][r]!=velNull)
       ||(numConvergingCenters[l][r]!=(int)velNull)||(meanVT[l][r]!=velNull)
       ||(meanVTUncertainty[l][r]!=velNull))
      return false;
    return true;
  }
  return false;
}

void SimplexData::printString()
{
 QTextStream out(stdout);
  out<< endl;
  out<< "Printing SimplexData Time = "+getTime().toString(Qt::ISODate)<< endl;
  out<< "  time: "+getTime().toString(Qt::ISODate) << endl;
  for(int i = 0; i < 2; i++) {
    QString ii = QString().setNum(i);
    out<<"  meanX @ level:"+ii+": "+QString().setNum(getX(i,0)) <<endl;
    out<<"  meanY @ level:"+ii+": "+QString().setNum(getY(i,0)) << endl;
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
