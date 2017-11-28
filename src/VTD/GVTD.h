/*
 *  GVTD.h
 *  vortrac
 *
 */

#ifndef GVTD_H
#define GVTD_H

#include <QString>
#include "DataObjects/Coefficient.h"

#include "VTD.h"

class GVTD : public VTD
{

public:
  
    GVTD(QString& initClosure, int& wavenumbers, float*& gaps, float hvvpwind);
    
    ~GVTD();

    bool analyzeRing(float& xCenter, float& yCenter, float& radius,
		     float& height, int& numData, float*& ringData,
		     float*& ringAzimuths, Coefficient*& vtdCoeffs,
		     float& stdDev);

    void  setWindCoefficients(float& radius, float& height,
			      int& numCoefficients, float*& FourierCoeffs,
			      Coefficient*& vtdCoeffs);
};

#endif

