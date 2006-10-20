/*
 *  VortexData.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 8/2/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef VORTEXDATA_H
#define VORTEXDATA_H

#include "Coefficient.h"
#include<QDateTime>

class VortexData
{

 public:
  VortexData();
  VortexData(int availLevels, int availRadii, int availWaveNum);
  //VortexData(const VortexData &other);
  ~VortexData();
  void printString();

  float getLat();
  float getLat(const int& i) const;
  void setLat(const int& index, const float& latitude);
  void setLat(const float a[], const int& howMany);

  float getLon();
  float getLon(const int& i) const;
  void setLon(const int& index,const float& longitude);
  void setLon(const float a[], const int& howMany);

  float getHeight(const int& i) const;
  void setHeight(const int& index, const float& altitude);
  void setHeight(const float a[], const int& howMany);

  QDateTime getTime() const;
  void setTime(const QDateTime& radartime);

  float getRMW();
  float getRMW(const int& i) const;
  void setRMW(const int& index, const float& new_rmw);
  void setRMW(const float a[], const int& howMany);
  float getRMWUncertainty();
  float getRMWUncertainty(const int& i) const;
  void setRMWUncertainty(const int& index, const float& dRMW);
  void setRMWUncertainty(const float a[], const int& howMany);

  float getPressure() const;
  void setPressure(const float& pressure);
  float getPressureUncertainty() const;
  void setPressureUncertainty(const float& dPressure);
  float getPressureDeficit() const;
  void setPressureDeficit(const float& newPressureDeficit);

  int getNumConvergingCenters(const int& i)const;
  void setNumConvergingCenters(const int& index, const int& number);
  void setNumConvergingCenters(const int a[], const int& howMany);

  float getCenterStdDev(const int& i) const;
  void setCenterStdDev(const int& index, const float& number);
  void setCenterStdDev(const float a[], const int& howMany);

 
  Coefficient getCoefficient(const int& lev, const int& rad, 
			     const int& waveNum) const;
  void setCoefficient(const int& lev, const int& rad, 
		      const int& waveNum, const Coefficient &coefficient);

  // void operator = (const VortexData &other);
  bool operator ==(const VortexData &other);
  bool operator < (const VortexData &other);
  bool operator > (const VortexData &other);

  int getNumLevels() const {return numLevels;}
  int getNumRadii() const {return numRadii;}
  int getNumWaveNum() const {return numWaveNum;}

 private:
  static const int maxLevels = 15;
  static const int maxRadii = 30;
  static const int maxWaveNum = 25;

  int numLevels;
  int numRadii;
  int numWaveNum;

  float centerLatitude[maxLevels];
  float centerLongitude[maxLevels];
  float centerAltitude[maxLevels];
  
  QDateTime time;

  float RMW[maxLevels];
  float RMWUncertainty[maxLevels];

  float centralPressure;
  float centralPressureUncertainty;
  float pressureDeficit;

  int numConvergingCenters[maxLevels];
  float centerStdDeviation[maxLevels];
  
  Coefficient coefficients[maxLevels][maxRadii][maxWaveNum];
  
};


#endif


