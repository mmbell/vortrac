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

#include<QDateTime>

class VortexData
{

 public:
  VortexData();
  ~VortexData();

  float getLat();
  float getLat(const int& i);
  void setLat(const int& index, const float& longitude);
  void setLat(const float a[], const int& howMany);

  float getLon();
  float getLon(const int& i);
  void setLon(const int& index,const float& longitude);
  void setLon(const float a[], const int& howMany);

  float getAltitude(const int& i);
  void setAltitude(const int& index, const float& altitude);
  void setAltitude(const float a[], const int& howMany);
  float getAltitudeUncertainty(const int& i);
  void setAltUncertainty(const int& index, const float& dAltitude);
  void setAltUncertainty(const float a[], const int& howMany);

  QDateTime getTime();
  void setTime(const QDateTime& radartime);

  float getRMW();
  float getRMW(const int& i);
  void setRMW(const int& index, const float& new_rmw);
  void setRMW(const float a[], const int& howMany);
  float getRMWUncertainty();
  float getRMWUncertainty(const int& i);
  void setRMWUncertainty(const int& index, const float& dRMW);
  void setRMWUncertainty(const float a[], const int& howMany);

  float getPressure();
  void setPressure(const float& pressure);
  float getPressureUncertainty();
  void setPressureUncertainty(const float& dPressure);

  int getNumCCenters(const int& i);
  void setNumCCenters(const int& index, const int& number);
  void setNumCCenters(const int a[], const int& howMany);

  float getWindCoefficents(const int& level, const int& radius, const int& waveNumber);
  void setWindCoefficents(const int& level, const int& radius, const int& waveNumber, const float&coefficent);
  float getReflectivityCoefficents(const int& level, const int& radius, const int& waveNumber);
  void setReflectivityCoefficents(const int& level, const int& radius, const int& waveNumber, const float&coefficent);


 private:
  static const int numLevels = 15;
  static const int numRadii = 30;
  static const int numWaveNum = 4;

  float centerLatitude[numLevels];
  float centerLongitude[numLevels];
  float centerAltitude[numLevels];
  float centerAltUncertainty[numLevels];

  QDateTime time;

  float RMW[numLevels];
  float RMWUncertainty[numLevels];

  float centralPressure;
  float centralPressureUncertainty;

  int numCCenters[numLevels];

  float windCoefficents[numLevels][numRadii][numWaveNum];
  float reflectivityCoefficents[numLevels][numRadii][numWaveNum];

};

#endif
