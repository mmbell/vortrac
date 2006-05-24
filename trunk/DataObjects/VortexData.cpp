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

VortexData::VortexData()
{
  for(int i = 0; i < numLevels; i++)
    {
      centerLatitude[i] = 0;
      centerLongitude[i] = 0;
      centerAltitude[i] = 0;
      RMW[i] = 0;
      RMWUncertainty[i] = 0;
      numConvergingCenters[i] = 0;
      centerStdDeviation[i] = 0;
      for(int j = 0; j < numRadii; j++) {
	for(int k = 0; k < 2*numWaveNum; k++) {
	  radialWinds[i][j][k] = Coefficient();
	  tangentialWinds[i][j][k] = Coefficient();
	  reflectivity[i][j][k] = Coefficient();
	}
      }
    }

  time = QDateTime();

  centralPressure = 0;
  centralPressureUncertainty = 0;

}

VortexData::VortexData(const VortexData &other) 
{
  
  for(int i = 0; i < numLevels; i++)
    {
      centerLatitude[i] = other.centerLatitude[i];
      centerLongitude[i] = other.centerLongitude[i];
      centerAltitude[i] = other.centerAltitude[i];
      RMW[i] = other.RMW[i];
      RMWUncertainty[i] = other.RMWUncertainty[i];
      numConvergingCenters[i] = other.numConvergingCenters[i];
      centerStdDeviation[i] = other.centerStdDeviation[i];
      for(int j = 0; j < numRadii; j++) {
	for(int k = 0; k < 2*numWaveNum; k++) {
	  radialWinds[i][j][k] = other.getRadial(i,j,k);
	  tangentialWinds[i][j][k] = other.getTangential(i,j,k);
	  reflectivity[i][j][k] = other.getReflectivity(i,j,k);
	}
      }
    }

  time = other.time;
  centralPressure = other.centralPressure;
  centralPressureUncertainty = other.centralPressureUncertainty;

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

float VortexData::getAltitude(const int& i) const
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

QDateTime VortexData::getTime() const
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

float VortexData::getRMWUncertainty()
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

Coefficient VortexData::getTangential(const int& lev, const int& rad, 
			  const int& waveNum) const
{
  return tangentialWinds[lev][rad][waveNum];
}

void VortexData::setTangential(const int& lev, const int& rad, 
		   const int& waveNum, const Coefficient &coefficient)
{
  tangentialWinds[lev][rad][waveNum] = coefficient;
}

Coefficient VortexData::getRadial(const int& lev, const int& rad, 
		      const int& waveNum) const
{
  return radialWinds[lev][rad][waveNum];
}

void VortexData::setRadial(const int& lev, const int& rad, 
	       const int& waveNum, const Coefficient &coefficient)
{
  radialWinds[lev][rad][waveNum] = coefficient;
}

Coefficient VortexData::getReflectivity(const int& lev, const int& rad,
					const int& waveNum) const
{
  return reflectivity[lev][rad][waveNum];
}

void VortexData::setReflectivity(const int& lev, const int& rad, 
				 const int& waveNum,
				 const Coefficient& coefficient)
{
  reflectivity[lev][rad][waveNum] = coefficient;
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

void VortexData:: operator = (const VortexData &other)
{
 
  for(int i = 0; i < numLevels; i++)
    {
      centerLatitude[i] = other.centerLatitude[i];
      centerLongitude[i] = other.centerLongitude[i];
      centerAltitude[i] = other.centerAltitude[i];
      RMW[i] = other.RMW[i];
      RMWUncertainty[i] = other.RMWUncertainty[i];
      numConvergingCenters[i] = other.numConvergingCenters[i];
      centerStdDeviation[i] = other.centerStdDeviation[i];
      for(int j = 0; j < numRadii; j++) {
	for(int k = 0; k < 2*numWaveNum; k++) {
	  radialWinds[i][j][k] = other.getRadial(i,j,k);
	  tangentialWinds[i][j][k] = other.getTangential(i,j,k);
	  reflectivity[i][j][k] = other.getReflectivity(i,j,k);
	}
      }
    }

  time = other.time;
  centralPressure = other.centralPressure;
  centralPressureUncertainty = other.centralPressureUncertainty;
}

void VortexData::printString()
{
  QTextStream out(stdout);
  out<< endl;
  out<< "Printing VortexData Time = "+getTime().toString()<< endl;
  out<< "  time: "+getTime().toString() << endl;
  out<< "  pressure: "+QString().setNum(getPressure()) << endl;
  out<< "  pressure uncertainty: ";
  out<<              QString().setNum(getPressureUncertainty())<<endl;
  for(int i = 0; i < 2; i++) {
    QString ii = QString().setNum(i);
    out<<"  altitude @ level:"+ii+": "+QString().setNum(getAltitude(i)) <<endl;
    out<<"  latitude @ level:"+ii+": "+QString().setNum(getLat(i)) << endl;
    out<<"  longitude @ level:"+ii+": "+QString().setNum(getLon(i)) << endl;
    out<<"  rmw @ level:"+ii+": "+QString().setNum(getRMW(i)) << endl;
    out<<"  rmw uncertainty @ level:"+ii+": ";
    out<<         QString().setNum(getRMWUncertainty(i)) << endl;
    out<<"  NumConvCenters @ level:"+ii+": ";
    out<<         QString().setNum(getNumConvergingCenters(i)) << endl;
    out<<"  CenterStdDev @ level:"+ii+": ";
    out<<         QString().setNum(getCenterStdDev(i)) << endl;
    out<<"  reflectivity @ level:"+ii+": ";
    out<<         QString().setNum(getReflectivity(i,0,0).getValue()) << endl;
    out<<"  tangential Winds @ level:"+ii+": ";
    out<<         QString().setNum(getTangential(i,0,0).getValue()) << endl;
    out<<"  radial Winds @ level:"+ii+": ";
    out<<         QString().setNum(getRadial(i,0,0).getValue()) << endl;
    out<<"  ----------------------------------------------------------" <<endl;
  }
  
}
