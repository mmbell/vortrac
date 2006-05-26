/*
 *  GBVTD.cpp
 *  vortrac
 *
 *  Created by Michael Bell on 5/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "GBVTD.h"
#include <math.h>

GBVTD::GBVTD(QString& initGeometry, QString& initClosure,int& maxWavenumber, float*& gaps)
{
	// Construct with data gaps
	geometry = initGeometry;
	closure = initClosure;
	wavenumbers = maxWavenumber;
	dataGaps = gaps;
	Pi = 3.141592653589793238462643;
	deg2rad = Pi/180.;
	rad2deg = 180./Pi;
	
}

GBVTD::~GBVTD()
{
	// Default destructor
}


bool GBVTD::analyzeRing(float& xCenter, float& yCenter, float& radius, int& numData, 
						float*& ringData, float*& ringAzimuths, Coefficient*& vtdCoeffs, float& vtdStdDev)
{

	// Analyze a ring of data
	if (geometry == "vtd") {
		// Make a Psi array
		ringPsi = new float[numData];
		vel = new float[numData];
		psi = new float[numData];
		
		// Get thetaT
		thetaT = atan2(yCenter,xCenter);
		thetaT = fixAngle(thetaT);
		for (int i=0; i <= numData-1; i++) {
			// Convert to Psi
			float angle = ringAzimuths[i]*deg2rad;
			angle = fixAngle(angle);
			float xx = xCenter + radius * cos(angle);
			float yy = yCenter + radius * sin(angle);
			float psiCorrection = atan2(yy,xx) - thetaT;
			ringPsi[i] = angle = psiCorrection;
		}
		
		// Get the maximum number of coefficients for the given data distribution and geometry
		//numCoefficients = getNumCoefficients(numData, ringPsi, ringData);
		
		// Threshold bad values
		int goodCount = 0;
		
		for (int i=0; i <= numData-1; i++) {
			if (ringData[i] != -999.) {
				// Good point
				vel[goodCount] = ringData[i];
				psi[goodCount] = ringPsi[i];
				goodCount++;
			}
		}
		numData = goodCount;
		
		// Least squares
	}
	
	return true;
			
}

float GBVTD::fixAngle(float& angle)
{
	
	// Make sure an angle is between 0 and 2Pi
	if (fabs(angle) < 1.0e-06) angle = 0.0;
	if (angle > (2*Pi)) angle = angle - 2*Pi;
	if (angle < 0.) angle = angle + 2*Pi;
	return angle;

}


