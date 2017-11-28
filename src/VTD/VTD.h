/*
 *  VTD.h
 *  vortrac
 *
 *  Abstract base class for several *VTD methods
 *
 *  Subclass and add an analyzeRing amd setWindCoefficients functions to use
 */

#ifndef VTD_H
#define VTD_H

#include <QString>
#include "DataObjects/Coefficient.h"

class VTD
{

 public:
  
  VTD(QString& initClosure, int& wavenumbers, float*& gaps,
      float hvvpwind);
    
  virtual ~VTD();

  // Analyze one ring of data
    
  virtual bool analyzeRing(float& xCenter, float& yCenter, float& radius,
			   float& height, int& numData, float*& ringData,
			   float*& ringAzimuths, Coefficient*& vtdCoeffs,
			   float& stdDev) = 0;

  virtual void  setWindCoefficients(float& radius, float& height,
				    int& numCoefficients, float*& FourierCoeffs,
				    Coefficient*& vtdCoeffs) = 0;
    
  void setHVVP(const float& meanWind);

  int   getNumCoefficients(int& numData);
  float fixAngle(float& angle);

 protected:
    
  static const float PI     ;
  static const float DEG2RAD;
  static const float RAD2DEG;

  QString geometry;
  QString closure;
  int _maxWaveNum;
  float* dataGaps;

  float* ringPsi;
  float* vel;
  float* psi;
  float thetaT;
  float centerDistance;
  float level;
  float* FourierCoeffs;

  float _hvvpMean;

};

#endif
