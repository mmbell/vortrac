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

#include "IO/Message.h"
#include "DataObjects/Coefficient.h"
#include "Math/Matrix.h"

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
		
	QString geometry;
	QString closure;
	int maxWavenumber;
	float* dataGaps;
	float Pi;
	float deg2rad;
	float rad2deg;
	float* ringPsi;
	float* thetaP;
	float* ringthetaP;
	float* D;
	float* vel;
	float* psi;
	float thetaT;
	float centerDistance;
	int numCoeffs;
	float level;
	float* FourierCoeffs;
	Matrix llsSolver;

	float hvvpMeanWind;
	
	void setWindCoefficients(float& radius, float& height, int& numCoefficients, float*& FourierCoeffs, Coefficient*& vtdCoeffs);
	int getNumCoefficients(int& numData);
	//int getNumCoefficients(int& numData,float& r, float& z);
	float fixAngle(float& angle);
	
};

#endif

