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
#include <QTextStream>
#include "Message.h"
#include <math.h>

VortexData::VortexData()
{
  numLevels = maxLevels;
  numRadii = maxRadii;
  numWaveNum = maxWaveNum;

  for(int i = 0; i < numLevels; i++)
    {
      centerLatitude[i] = -999;
      centerLongitude[i] = -999;
      centerAltitude[i] = -999;
      RMW[i] = -999;
      RMWUncertainty[i] = -999;
      numConvergingCenters[i] = -999;
      centerStdDeviation[i] = -999;
      for(int j = 0; j < numRadii; j++) {
	for(int k = 0; k < numWaveNum; k++) {
	  coefficients[i][j][k] = Coefficient();
	}
      }
    }

  time = QDateTime();

  centralPressure = -999;
  centralPressureUncertainty = -999;
  pressureDeficit = -999;

}

VortexData::VortexData(int availLevels, int availRadii, int availWaveNum)
{
  numLevels = availLevels;
  numRadii = availRadii;
  numWaveNum = availWaveNum;
  
  for(int i = 0; i < numLevels; i++)
    {
      centerLatitude[i] = -999;
      centerLongitude[i] = -999;
      centerAltitude[i] = -999;
      RMW[i] = -999;
      RMWUncertainty[i] = -999;
      numConvergingCenters[i] = -999;
      centerStdDeviation[i] = -999;
      for(int j = 0; j < numRadii; j++) {
	for(int k = 0; k < numWaveNum; k++) {
	  coefficients[i][j][k] = Coefficient();
	}
      }
    }
  
  time = QDateTime();
  
  centralPressure = -999;
  centralPressureUncertainty = -999;
  pressureDeficit = -999;
  
}


VortexData::~VortexData()
{
}

float VortexData::getLat()
{
  return centerLatitude[0];
}

float VortexData::getLat(const int& i) const
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

float VortexData::getLon(const int& i) const
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

float VortexData::getHeight(const int& i) const
{
  if (i < numLevels)
    return centerAltitude[i];
  return centerAltitude[0];
}

int VortexData::getHeightIndex(const float& height) const
{

  // Takes height in KM
  if(centerAltitude[0]==-999) {
    Message::toScreen("VORTEXDATA:NO HEIGHT DATA IS IN SYSTEM!!!!");
    return -1;
  }
 
  float heightDiff = 100;
  int closestIndex = -1;
  
  for(int i = 0; i < numLevels; i++) {
    if(centerAltitude[i] == -999) { continue; }
    if(fabs(height - centerAltitude[i]) < heightDiff) {
      heightDiff = fabs(height-centerAltitude[i]);
      closestIndex = i;
    }
  }
  if(closestIndex == -1) {
    Message::toScreen("VORTEXDATA:UNABLE TO FIND SUITABLE INDEX");
    return -1;
  }
  return closestIndex;
}

void VortexData::setHeight(const int& index, const float& altitude)
{
  if (index < numLevels)
    centerAltitude[index] = altitude;
}

void VortexData::setHeight(const float a[],const int& howMany)
{
  for (int i = 0; i < howMany; i++)
    centerAltitude[i] = a[i];
}

QDateTime VortexData::getTime() const
{
  return time;
}

void VortexData::setTime(const QDateTime& radarTime)
{
  time = QDateTime(radarTime);
}


float VortexData::getRMW() const
{
  return RMW[0];
}

float VortexData::getRMW(const int& i) const
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

float VortexData::getRMWUncertainty() const
{
  return RMWUncertainty[0];
}

float VortexData::getRMWUncertainty(const int& i) const
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


float VortexData::getPressure() const
{
  return centralPressure;
}

void VortexData::setPressure(const float& pressure)
{
  centralPressure = pressure;
}

float VortexData::getPressureUncertainty() const
{
  return centralPressureUncertainty;
}

void VortexData::setPressureUncertainty(const float& dPressure)
{
  centralPressureUncertainty = dPressure;
}

float VortexData::getPressureDeficit() const
{
  return pressureDeficit;
}

void VortexData::setPressureDeficit(const float& newPressureDeficit)
{
  pressureDeficit = newPressureDeficit;
}

int VortexData::getNumConvergingCenters(const int& i) const
{
  if(i < numLevels)
    return numConvergingCenters[i];
  return numConvergingCenters[0];
}

void VortexData::setNumConvergingCenters(const int& index, const int& number)
{
  numConvergingCenters[index] = number;
}

void VortexData::setNumConvergingCenters(const int a[], const int& howMany)
{
  for(int i = 0; i < howMany; i++)
    numConvergingCenters[i] = a[i];
}

float VortexData::getCenterStdDev(const int& i) const
{
  if(i < numLevels)
    return centerStdDeviation[i];
  return centerStdDeviation[0];
}

void VortexData::setCenterStdDev(const int& index, const float& number)
{
  centerStdDeviation[index] = number;
}

void VortexData::setCenterStdDev(const float a[], const int& howMany)
{
  for(int i = 0; i < howMany; i++)
    centerStdDeviation[i]=a[i];
}

Coefficient VortexData::getCoefficient(const int& lev, const int& rad, 
				       const int& waveNum) const
{
  return coefficients[lev][rad][waveNum];
}

Coefficient VortexData::getCoefficient(const int& lev, const int& rad,
				       const QString& parameter) const
{
  for(int i = 0; i < numWaveNum; i++) {
    if(coefficients[lev][rad][i].getParameter()==parameter)
      return coefficients[lev][rad][i];
  }
  return Coefficient();
}

Coefficient VortexData::getCoefficient(const float& height, const int& rad,
				       const QString& parameter) const
{
  int level = getHeightIndex(height);
  //int radIndex = getCoeffRadiusIndex(height,rad);
  if((level == -1)||(rad == -1)) {
    Message::toScreen("VortexData: GetCoefficient: Can't Get Needed Indices");
    return Coefficient();
  }
  return getCoefficient(level, rad, parameter);
}

void VortexData::setCoefficient(const int& lev, const int& rad, 
	       const int& waveNum, const Coefficient &coefficient)
{
  coefficients[lev][rad][waveNum] = coefficient;
}

bool VortexData::operator ==(const VortexData &other)
{
  if(this->time == other.time)
    return true;
  return false;
}

bool VortexData::operator < (const VortexData &other)
{
  if(this->time < other.time)
    return true;
  return false;
}

bool VortexData::operator > (const VortexData &other)
{
  if(this->time > other.time)
    return true;
  return false;
}


void VortexData::printString()
{
  QTextStream out(stdout);
  out<< endl;
  out<< "Printing VortexData Time = "+getTime().toString(Qt::ISODate)<< endl;
  out<< "  time: "+getTime().toString(Qt::ISODate) << endl;
  out<< "  pressure: "+QString().setNum(getPressure()) << endl;
  out<< "  pressure uncertainty: ";
  out<<              QString().setNum(getPressureUncertainty())<<endl;
  for(int i = 0; i < 2; i++) {
    QString ii = QString().setNum(i);
    out<<"  altitude @ level:"+ii+": "+QString().setNum(getHeight(i)) <<endl;
    out<<"  latitude @ level:"+ii+": "+QString().setNum(getLat(i)) << endl;
    out<<"  longitude @ level:"+ii+": "+QString().setNum(getLon(i)) << endl;
    out<<"  rmw @ level:"+ii+": "+QString().setNum(getRMW(i)) << endl;
    out<<"  rmw uncertainty @ level:"+ii+": ";
    out<<         QString().setNum(getRMWUncertainty(i)) << endl;
    out<<"  NumConvCenters @ level:"+ii+": ";
    out<<         QString().setNum(getNumConvergingCenters(i)) << endl;
    out<<"  CenterStdDev @ level:"+ii+": ";
    out<<         QString().setNum(getCenterStdDev(i)) << endl;
    out<<"  coefficients @ level:"+ii+": ";
    out<<         QString().setNum(getCoefficient(i,0,0).getValue()) << endl;
    out<<"  ----------------------------------------------------------" <<endl;
  }
  
}

void VortexData::setNumLevels(const int& num)
{
  if(num <= maxLevels)
    numLevels = num;
}

void VortexData::setNumRadii(const int& num)
{
  if(num <= maxRadii)
    numRadii = num;
}

void VortexData::setNumWaveNum(const int& num)
{
  if(num <= maxWaveNum)
    numWaveNum = num;
}
