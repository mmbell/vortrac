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
  VortexData(const VortexData &other);
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

  float getAltitude(const int& i) const;
  void setAltitude(const int& index, const float& altitude);
  void setAltitude(const float a[], const int& howMany);

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

  int getNumConvergingCenters(const int& i)const;
  void setNumConvergingCenters(const int& index, const int& number);
  void setNumConvergingCenters(const int a[], const int& howMany);

  float getCenterStdDev(const int& i) const;
  void setCenterStdDev(const int& index, const float& number);
  void setCenterStdDev(const float a[], const int& howMany);

  Coefficient getTangential(const int& lev, const int& rad, 
				const int& waveNum) const;
  void setTangential(const int& lev, const int& rad, 
			  const int& waveNum, const Coefficient &coefficient);
  Coefficient getRadial(const int& lev, const int& rad, 
			    const int& waveNum) const;
  void setRadial(const int& lev, const int& rad, 
		      const int& waveNum, const Coefficient &coefficient);
  Coefficient getReflectivity(const int& lev, const int& rad,
			      const int& waveNum) const;
  void setReflectivity(const int& lev, const int& rad, const int& waveNum,
		       const Coefficient& coefficient);

  void operator = (const VortexData &other);
  bool operator ==(const VortexData &other);
  bool operator < (const VortexData &other);
  bool operator > (const VortexData &other);

  int getNumLevels() const {return numLevels;}
  int getNumRadii() const {return numRadii;}
  int getNumWaveNum() const {return numWaveNum;}

 private:
  static const int numLevels = 15;
  static const int numRadii = 30;
  static const int numWaveNum = 4;

  float centerLatitude[numLevels];
  float centerLongitude[numLevels];
  float centerAltitude[numLevels];
  
  QDateTime time;

  float RMW[numLevels];
  float RMWUncertainty[numLevels];

  float centralPressure;
  float centralPressureUncertainty;

  int numConvergingCenters[numLevels];
  float centerStdDeviation[numLevels];
  
  Coefficient radialWinds[numLevels][numRadii][2*numWaveNum];
  Coefficient tangentialWinds[numLevels][numRadii][2*numWaveNum];
  Coefficient reflectivity[numLevels][numRadii][2*numWaveNum];
  
};


#endif


