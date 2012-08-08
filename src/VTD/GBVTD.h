/*
 *  GBVTD.h
 *  vortrac
 *
 *  Created by Michael Bell on 5/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef GBVTD_H
#define GBVTD_H

#include <QString>
#include "DataObjects/Coefficient.h"

class GBVTD
{

public:
    GBVTD(QString& initGeometry, QString& initClosure, int& wavenumbers, float*& gaps);
    GBVTD(QString& initGeometry, QString& initClosure, int& wavenumbers, float*& gaps, float& hvvpwind);
    ~GBVTD();

    bool analyzeRing(float& xCenter, float& yCenter, float& radius, float& height, int& numData,
                     float*& ringData, float*& ringAzimuths, Coefficient*& vtdCoeffs, float& stdDev);
    void setHVVP(const float& meanWind);

private:
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

    void  setWindCoefficients(float& radius, float& height, int& numCoefficients, float*& FourierCoeffs, Coefficient*& vtdCoeffs);
    int   getNumCoefficients(int& numData);
    float fixAngle(float& angle);

};

#endif

