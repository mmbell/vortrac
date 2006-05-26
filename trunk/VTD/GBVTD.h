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

class GBVTD
{
	
public:
	GBVTD(QString& initGeometry, QString& initClosure, int& maxWavenumber, float*& gaps);
	~GBVTD();
	
	bool analyzeRing(float& xCenter, float& yCenter, float& radius, int& numData,
					 float*& ringData, float*& ringAzimuths, Coefficient*& vtdCoeffs, float& stdDev);
private:
		
	QString geometry;
	QString closure;
	float wavenumbers;
	float* dataGaps;
	float Pi;
	float deg2rad;
	float rad2deg;
	float* ringPsi;
	float* vel;
	float* psi;
	float thetaT;
	
	float fixAngle(float& angle);
	
};

#endif

