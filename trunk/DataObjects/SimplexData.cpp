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

SimplexData::SimplexData()
{
  numLevels = maxLevels;
  numRadii = maxRadii;
  numCenters = maxCenters;

  for(int i = 0; i < numLevels; i++)
    {
      height[i] = -999;
      for(int j = 0; j < numRadii; j++) {
	meanX[i][j] = -999;
	meanY[i][j] = -999;
	centerStdDeviation[i][j] = -999.;
	meanVT[i][j] = -999;
	meanVTUncertainty[i][j] = -999;
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
  
  for(int i = 0; i < numLevels; i++)
    {
      height[i] = -999;
      for(int j = 0; j < numRadii; j++) 
	{
	  meanX[i][j] = -999;
	  meanY[i][j] = -999;
	  centerStdDeviation[i][j] = -999;
	  meanVT[i][j] = -999;
	  meanVTUncertainty[i][j] = -999;
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



Center SimplexData::getCenter(const int& lev, const int& rad, 
			      const int& waveNum) const
{
  return centers[lev][rad][waveNum];
}

void SimplexData::setCenter(const int& lev, const int& rad, 
	       const int& waveNum, const Center &newCenter)
{
  centers[lev][rad][waveNum] = newCenter;
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

