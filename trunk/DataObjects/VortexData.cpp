/*
 *  VortexData.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 8/02/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "VortexData.h"

VortexData::VortexData()
{
  for(int i = 0; i < numLevels; i++)
    {
      centerLatitude[i] = 0;
      centerLongitude[i] = 0;
      centerAltitude[i] = 0;
      centerAltUncertainty[i] = 0;
      RMW[i] = 0;
      RMWUncertainty[i] = 0;
      numCCenters[i] = 0;
    }

  time = QDateTime();

  centralPressure = 0;
  centralPressureUncertainty = 0;

}

VortexData::~VortexData()
{
}

float VortexData::getLat()
{
  return centerLatitude[0];
}

float VortexData::getLat(const int& i)
{
  if (i < numLevels)
    return centerLatitude[i];
  return centerLatitude[0];
}

void VortexData::setLat(const int& index, const float& latitude)
{
  if (index < numLevels)
    centerLatitude[index] = latitude;
}

void VortexData::setLat(const float a[], const int& howMany)
{
  for (int i = 0; i < howMany; i++)
    centerLatitude[i] = a[i];
}

float VortexData::getLon()
{
  return centerLongitude[0];
}

float VortexData::getLon(const int& i)
{
  if (i < numLevels)
    return centerLongitude[i];
  return centerLongitude[0];
}

void VortexData::setLon(const int& index, const float& longitude)
{
  if (index < numLevels)
    centerLongitude[index] = longitude;
}

void VortexData::setLon(const float a[], const int& howMany)
{
  for (int i = 0; i < howMany; i ++)
    centerLongitude[i] = a[i];
}

float VortexData::getAltitude(const int& i)
{
  if (i < numLevels)
    return centerAltitude[i];
  return centerAltitude[0];
}


void VortexData::setAltitude(const int& index, const float& altitude)
{
  if (index < numLevels)
    centerAltitude[index] = altitude;
}

void VortexData::setAltitude(const float a[],const int& howMany)
{
  for (int i = 0; i < howMany; i++)
    centerAltitude[i] = a[i];
}

float VortexData::getAltitudeUncertainty(const int& i)
{
  if (i < numLevels)
    return centerAltUncertainty[i];
  return centerAltUncertainty[0];
}

void VortexData::setAltUncertainty(const int& index, const float& dAltitude)
{
  if (index < numLevels)
    centerAltUncertainty[index] = dAltitude;
}

void VortexData::setAltUncertainty(const float a[], const int& howMany)
{
  for ( int i = 0; i < howMany; i++)
    centerAltUncertainty[i] = a[i];
}

QDateTime VortexData::getTime()
{
  return time;
}

void VortexData::setTime(const QDateTime& radarTime)
{
  time = QDateTime(radarTime);
}


float VortexData::getRMW()
{
  return RMW[0];
}

float VortexData::getRMW(const int& i)
{
  if (i < numLevels)
    return RMW[i];
  return RMW[0];
}

void VortexData::setRMW(const int& index, const float& new_rmw)
{
  if (index < numLevels)
    RMW[index] = new_rmw;
}

void VortexData::setRMW(const float a[], const int& howMany)
{
  for ( int i = 0; i < howMany; i++)
    RMW[i] = a[i];
}

float VortexData::getRMWUncertainty()
{
  return RMWUncertainty[0];
}

float VortexData::getRMWUncertainty(const int& i)
{
  if(i < numLevels)
    return RMWUncertainty[i];
  return RMWUncertainty[0];
}

void VortexData::setRMWUncertainty(const int& index, const float& dRMW)
{
  if (index < numLevels)
    RMWUncertainty[index] = dRMW;
}

void VortexData::setRMWUncertainty(const float a[], const int& howMany)
{
  for (int i = 0; i < howMany; i++)
    RMWUncertainty[i] = a[i];
}


float VortexData::getPressure()
{
  return centralPressure;
}

void VortexData::setPressure(const float& pressure)
{
  centralPressure = pressure;
}

float VortexData::getPressureUncertainty()
{
  return centralPressureUncertainty;
}

void VortexData::setPressureUncertainty(const float& dPressure)
{
  centralPressureUncertainty = dPressure;
}

int VortexData::getNumCCenters(const int& i)
{
  if(i < numLevels)
    return numCCenters[i];
  return numCCenters[0];
}

void VortexData::setNumCCenters(const int& index, const int& number)
{
  numCCenters[index] = number;
}

void VortexData::setNumCCenters(const int a[], const int& howMany)
{
  for(int i = 0; i < howMany; i++)
    numCCenters[i] = a[i];
}


float VortexData::getWindCoefficents(const int& level, const int& radius, const int& waveNumber)
{
  if ((level < numLevels)&&(radius<numRadii)&&(waveNumber< numWaveNum))
    return windCoefficents[level][radius][waveNumber];
  return 0;
}

void VortexData::setWindCoefficents(const int& level, const int& radius, const int& waveNumber, const float& coefficent)
{
  if ((level < numLevels)&&(radius<numRadii)&&(waveNumber< numWaveNum))
    windCoefficents[level][radius][waveNumber] = coefficent;
}

float VortexData::getReflectivityCoefficents(const int& level, const int& radius, const int& waveNumber)
{
  if ((level < numLevels)&&(radius<numRadii)&&(waveNumber< numWaveNum))
    return reflectivityCoefficents[level][radius][waveNumber];
  return 0;
}

void VortexData::setReflectivityCoefficents(const int& level, const int& radius, const int& waveNumber, const float& coefficent)
{
  if ((level < numLevels)&&(radius<numRadii)&&(waveNumber< numWaveNum))
    reflectivityCoefficents[level][radius][waveNumber] = coefficent;
}
