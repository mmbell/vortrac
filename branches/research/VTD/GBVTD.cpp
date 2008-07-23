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
#include "IO/Message.h"

GBVTD::GBVTD(QString& initGeometry, QString& initClosure,int& wavenumbers, float*& gaps)
{
	// Construct with data gaps
	geometry = initGeometry;
	closure = initClosure;
	maxWavenumber = wavenumbers;
	dataGaps = gaps;
	Pi = 3.141592653589793238462643;
	deg2rad = Pi/180.;
	rad2deg = 180./Pi;
	FourierCoeffs = new float[maxWavenumber*2 + 3];
	hvvpMeanWind = 0;
}

GBVTD::GBVTD(QString& initGeometry, QString& initClosure, int& wavenumbers, 
	     float*& gaps, float& hvvpwind)
{
	// Construct with data gaps
	geometry = initGeometry;
	closure = initClosure;
	maxWavenumber = wavenumbers;
	dataGaps = gaps;
	Pi = 3.141592653589793238462643;
	deg2rad = Pi/180.;
	rad2deg = 180./Pi;
	FourierCoeffs = new float[maxWavenumber*2 + 3];
	hvvpMeanWind = 0; 
	this->setHVVP(hvvpwind);
}

GBVTD::~GBVTD()
{
	// Default destructor
	delete[] FourierCoeffs;
	
}


bool GBVTD::analyzeRing(float& xCenter, float& yCenter, float& radius, float& height, int& numData, 
						float*& ringData, float*& ringAzimuths, Coefficient*& vtdCoeffs, float& vtdStdDev)
{

	// Analyze a ring of data
	if (geometry == "GBVTD") {
		// Make a Psi array
		ringPsi = new float[numData];
		vel = new float[numData];
		psi = new float[numData];
		
		// Get thetaT
		thetaT = atan2(yCenter,xCenter);
		thetaT = fixAngle(thetaT);
		centerDistance = sqrt(xCenter*xCenter + yCenter*yCenter);
		
		for (int i=0; i <= numData-1; i++) {
			// Convert to Psi
			float angle = ringAzimuths[i]*deg2rad - thetaT;
			angle = fixAngle(angle);
			float xx = xCenter + radius * cos(angle+thetaT);
			float yy = yCenter + radius * sin(angle+thetaT);
			float psiCorrection = atan2(yy,xx) - thetaT;
			ringPsi[i] = angle - psiCorrection;
			ringPsi[i] = fixAngle(ringPsi[i]);
			/* QString testvalues = QString::number(i) + " " + QString::number(ringData[i]) + " " 
				+ QString::number(ringPsi[i]) + " " + QString::number(angle);
			Message::toScreen(testvalues); */
		}
		

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
		
		// Get the maximum number of coefficients for the given data distribution and geometry
		numCoeffs = getNumCoefficients(numData);
		
		if (numCoeffs == 0) {
			// Too much missing data, set everything to 0 and return
			for (int i=0; i<=(maxWavenumber*2 + 2); i++) {
				FourierCoeffs[i] = 0.;
			}
			vtdStdDev = -999;
			setWindCoefficients(radius, height, numCoeffs, FourierCoeffs, vtdCoeffs, xCenter, yCenter);
			delete[] ringPsi;
			delete[] vel;
			delete[] psi;		
			return false;
		}
		
		// Least squares
		float** xLLS = new float*[numCoeffs];
		for (int i = 0; i<=numCoeffs-1; i++) {
			xLLS[i] = new float[numData];
		}
		float* yLLS = new float[numData];
		for (int i=0; i <= numData-1; i++) {
			xLLS[0][i] = 1.;
			for (int j=1; j <= (numCoeffs/2); j++) {
				xLLS[2*j-1][i] = sin(float(j)*psi[i]);
				xLLS[2*j][i] = cos(float(j)*psi[i]);
			}
			yLLS[i] = vel[i];
		}
		
		float* stdError = new float[numCoeffs];
		if(!llsSolver.lls(numCoeffs, numData, xLLS, yLLS, vtdStdDev, FourierCoeffs, stdError)) {

		  Message::toScreen("GBVTD Returned Nothing from LLS");
			//emit log(Message("Failed in lls"));
			for (int i = 0; i<=numCoeffs-1; i++)
				delete[] xLLS[i];
			delete[] yLLS;
			delete[] stdError;
			delete[] ringPsi;
			delete[] vel;
			delete[] psi;		
			return false;
		}
		
		// Convert Fourier coefficients into wind coefficients
		setWindCoefficients(radius, height, numCoeffs, FourierCoeffs, vtdCoeffs, xCenter, yCenter);
		for (int i = 0; i<=numCoeffs-1; i++) 
			delete[] xLLS[i];
		delete[] xLLS;
		delete[] yLLS;
		delete[] stdError;
		delete[] ringPsi;
		delete[] vel;
		delete[] psi;		
		
	}

	// Analyze a ring of data for the GVTD method
	if (geometry == "GVTD") {
		// Make vel and thetap arrays
		ringthetaP = new float[numData];
		vel = new float[numData];
		thetaP = new float[numData];
		D = new float[numData];
		
		// Get thetaT
		thetaT = atan2(yCenter,xCenter);
		thetaT = fixAngle(thetaT);
		centerDistance = sqrt(xCenter*xCenter + yCenter*yCenter);
		
		for (int i=0; i <= numData-1; i++) {
			// Assign Thetaprime and vel
			float angle = ringAzimuths[i]*deg2rad - thetaT;
			angle = fixAngle(angle);
			ringthetaP[i] = angle;
			float xx = xCenter + radius * cos(angle+thetaT);
			float yy = yCenter + radius * sin(angle+thetaT);
			D[i] = sqrt(xx*xx + yy*yy);
			//float psiCorrection = atan2(yy,xx) - thetaT;
			//ringPsi[i] = angle - psiCorrection;
			//ringPsi[i] = fixAngle(ringPsi[i]);
			/* QString testvalues = QString::number(i) + " " + QString::number(ringData[i]) + " " 
				+ QString::number(ringPsi[i]) + " " + QString::number(angle);
			Message::toScreen(testvalues); */
		}
		

		// Threshold bad values
		int goodCount = 0;
		
		for (int i=0; i <= numData-1; i++) {
			if (ringData[i] != -999.) {
				// Good point
				vel[goodCount] = ringData[i]*D[i]/centerDistance;
				thetaP[goodCount] = ringthetaP[i];

				//	Message::toScreen(QString().setNum(radius)+" "+QString().setNum(vel[goodCount])+" "+QString().setNum(thetaP[goodCount]));
				
				goodCount++;
			}
		}
		numData = goodCount;		
		
		// Get the maximum number of coefficients for the given data distribution and geometry

		
// 		if ( (radius == 79) && (height == 9) ) {
// 		  goodCount = numData;
// 		}

		numCoeffs = getNumCoefficients(numData);
		
		if (numCoeffs == 0) {
			// Too much missing data, set everything to 0 and return
			for (int i=0; i<=(maxWavenumber*2 + 2); i++) {
				FourierCoeffs[i] = 0.;
			}
			vtdStdDev = -999;
			setWindCoefficients(radius, height, numCoeffs, FourierCoeffs, vtdCoeffs, xCenter, yCenter);
			delete[] ringthetaP;
			delete[] vel;
			delete[] thetaP;		
			return false;
		}
		
		// Least squares
		float** xLLS = new float*[numCoeffs];
		for (int i = 0; i<=numCoeffs-1; i++) {
			xLLS[i] = new float[numData];
		}
		float* yLLS = new float[numData];
		for (int i=0; i <= numData-1; i++) {
			xLLS[0][i] = 1.;
			for (int j=1; j <= (numCoeffs/2); j++) {
				xLLS[2*j-1][i] = sin(float(j)*thetaP[i]);
				xLLS[2*j][i] = cos(float(j)*thetaP[i]);
			}
			yLLS[i] = vel[i];
		}
		
		float* stdError = new float[numCoeffs];
		if(!llsSolver.lls(numCoeffs, numData, xLLS, yLLS, vtdStdDev, FourierCoeffs, stdError)) {

		  Message::toScreen("GVTD Returned Nothing from LLS");
			//emit log(Message("Failed in lls"));
			for (int i = 0; i<=numCoeffs-1; i++)
				delete[] xLLS[i];
			delete[] yLLS;
			delete[] stdError;
			delete[] ringthetaP;
			delete[] vel;
			delete[] thetaP;		
			return false;
		}
		
		// Convert Fourier coefficients into wind coefficients
		setWindCoefficients(radius, height, numCoeffs, FourierCoeffs, vtdCoeffs, xCenter, yCenter);
		for (int i = 0; i<=numCoeffs-1; i++) 
			delete[] xLLS[i];
		delete[] xLLS;
		delete[] yLLS;
		delete[] stdError;
		delete[] ringthetaP;
		delete[] vel;
		delete[] thetaP;		
		
	}
	
	return true;
			
}

float GBVTD::analyzeMeanWind(float& xCenter, float& yCenter, float& radius, float& height, int& numData, 
						float*& ringData, float*& ringAzimuths, Coefficient*& vtdCoeffs, float& vtdStdDev)
{
        float VMCOS;

        // Analyze a ring of data for the GVTD method
	if (geometry == "GVTD") {
		// Make vel and thetap arrays
		ringthetaP = new float[numData];
		vel = new float[numData];
		thetaP = new float[numData];
		D = new float[numData];
		
		// Get thetaT
		thetaT = atan2(yCenter,xCenter);
		thetaT = fixAngle(thetaT);
		centerDistance = sqrt(xCenter*xCenter + yCenter*yCenter);
		
		for (int i=0; i <= numData-1; i++) {
			// Assign Thetaprime and vel
			float angle = ringAzimuths[i]*deg2rad - thetaT;
			angle = fixAngle(angle);
			ringthetaP[i] = angle;
			float xx = xCenter + radius * cos(angle+thetaT);
			float yy = yCenter + radius * sin(angle+thetaT);
			D[i] = sqrt(xx*xx + yy*yy);
			//float psiCorrection = atan2(yy,xx) - thetaT;
			//ringPsi[i] = angle - psiCorrection;
			//ringPsi[i] = fixAngle(ringPsi[i]);
			/* QString testvalues = QString::number(i) + " " + QString::number(ringData[i]) + " " 
				+ QString::number(ringPsi[i]) + " " + QString::number(angle);
			Message::toScreen(testvalues); */
		}
		

		// Threshold bad values
		int goodCount = 0;
		
		for (int i=0; i <= numData-1; i++) {
			if (ringData[i] != -999.) {
				// Good point
				vel[goodCount] = ringData[i]*D[i]/centerDistance;
				thetaP[goodCount] = ringthetaP[i];
				goodCount++;
			}
		}
		numData = goodCount;		
		
		// Get the maximum number of coefficients for the given data distribution and geometry

		
// 		if ( (radius == 79) && (height == 9) ) {
// 		  goodCount = numData;
// 		}

		numCoeffs = getNumCoefficients(numData);
		
		if (numCoeffs == 0) {
			// Too much missing data, set everything to 0 and return
			for (int i=0; i<=(maxWavenumber*2 + 2); i++) {
				FourierCoeffs[i] = 0.;
			}
			vtdStdDev = -999;
			setMeanWindCoefficients(radius, height, numCoeffs, FourierCoeffs, vtdCoeffs, xCenter, yCenter);
			delete[] ringthetaP;
			delete[] vel;
			delete[] thetaP;		
			return false;
		}
		
		// Least squares
		float** xLLS = new float*[numCoeffs];
		for (int i = 0; i<=numCoeffs-1; i++) {
			xLLS[i] = new float[numData];
		}
		float* yLLS = new float[numData];
		for (int i=0; i <= numData-1; i++) {
			xLLS[0][i] = 1.;
			for (int j=1; j <= (numCoeffs/2); j++) {
				xLLS[2*j-1][i] = sin(float(j)*thetaP[i]);
				xLLS[2*j][i] = cos(float(j)*thetaP[i]);
			}
			yLLS[i] = vel[i];
		}
		
		float* stdError = new float[numCoeffs];
		if(!llsSolver.lls(numCoeffs, numData, xLLS, yLLS, vtdStdDev, FourierCoeffs, stdError)) {

		  Message::toScreen("GVTD Returned Nothing from LLS");
			//emit log(Message("Failed in lls"));
			for (int i = 0; i<=numCoeffs-1; i++)
				delete[] xLLS[i];
			delete[] yLLS;
			delete[] stdError;
			delete[] ringthetaP;
			delete[] vel;
			delete[] thetaP;		
			return false;
		}
		
		// Convert Fourier coefficients into wind coefficients
		VMCOS = setMeanWindCoefficients(radius, height, numCoeffs, FourierCoeffs, vtdCoeffs, xCenter, yCenter);
		for (int i = 0; i<=numCoeffs-1; i++) 
			delete[] xLLS[i];
		delete[] xLLS;
		delete[] yLLS;
		delete[] stdError;
		delete[] ringthetaP;
		delete[] vel;
		delete[] thetaP;		
		
	}
	
	return VMCOS;
			
}

void GBVTD::setWindCoefficients(float& radius, float& level, int& numCoeffs, float*& FourierCoeffs, Coefficient*& vtdCoeffs, float xcenter, float ycenter)
{
	
	// Allocate and initialize the A & B coefficient arrays
	float* A;
	float* B;
	int maxIndex = numCoeffs/2 + 1;
	if (maxIndex > 5) {		
		A = new float[numCoeffs/2 + 1];
		B = new float[numCoeffs/2 + 1];
	} else {
		A = new float[5];
		B = new float[5];
	}
	for (int i=0; i <= 4; i++) {
		A[i] = 0;
		B[i] = 0;
	}
		
	float sinAlphamax = radius/centerDistance;
	float cosAlphamax = sqrt(centerDistance * centerDistance - radius*radius)/centerDistance;
	A[0] = FourierCoeffs[0];
	B[0] = 0.;
	for (int i=1; i <= (numCoeffs/2); i++) {
		A[i] = FourierCoeffs[2*i];
		B[i] = FourierCoeffs[2*i-1];
	}
	
	// Use the specified closure method to set VT, VR, and VM		
	if (closure.contains(QString("original"),Qt::CaseInsensitive)) {
		vtdCoeffs[0].setLevel(level);
		vtdCoeffs[0].setRadius(radius);
		vtdCoeffs[0].setParameter("VTC0");
		float value;
		if(closure.contains(QString("hvvp"),Qt::CaseInsensitive) and 
		   (B[1] != 0)) {
		  value = -B[1]-B[3]-hvvpMeanWind*sinAlphamax;
		}
		else {
		  value = -B[1]-B[3];
		}
		vtdCoeffs[0].setValue(value);
		
		vtdCoeffs[1].setLevel(level);
		vtdCoeffs[1].setRadius(radius);
		vtdCoeffs[1].setParameter("VRC0");
		value = A[1]+A[3];
		vtdCoeffs[1].setValue(value);

		vtdCoeffs[2].setLevel(level);
		vtdCoeffs[2].setRadius(radius);
		vtdCoeffs[2].setParameter("VMC0");
		value = A[0]+A[2]+A[4];
		vtdCoeffs[2].setValue(value);
		
		vtdCoeffs[3].setLevel(level);
		vtdCoeffs[3].setRadius(radius);
		vtdCoeffs[3].setParameter("VTS1");
		if ((sinAlphamax < 0.8) and (numCoeffs >= 5)) {
			value = A[2]-A[0]+A[4]+(A[0]+A[2]+A[4])*cosAlphamax;
			if (value < vtdCoeffs[0].getValue()) {
				vtdCoeffs[3].setValue(value);
			} else {
				vtdCoeffs[3].setValue(value);
			}
		} else {
			vtdCoeffs[3].setValue(0);
		}
		
		vtdCoeffs[4].setLevel(level);
		vtdCoeffs[4].setRadius(radius);
		vtdCoeffs[4].setParameter("VTC1");
		if ((sinAlphamax < 0.8) and (numCoeffs >= 5)) {
			value = -2.*(B[2]+B[4]);
			if (value < vtdCoeffs[0].getValue()) {
				vtdCoeffs[4].setValue(value);
			} else {
				vtdCoeffs[4].setValue(value);
			}
		} else {
			vtdCoeffs[4].setValue(0);
		}
		for (int i=5; i <= numCoeffs-1; i+=2) {
			vtdCoeffs[i].setLevel(level);
			vtdCoeffs[i].setRadius(radius);
			QString param = "VTC" + QString().setNum(int(i/2));
			vtdCoeffs[i].setParameter(param);
			value = -2*B[i/2+1];
			vtdCoeffs[i].setValue(value);	
			
			vtdCoeffs[i+1].setLevel(level);
			vtdCoeffs[i+1].setRadius(radius);
			param = "VTS" + QString().setNum(int(i/2));
			vtdCoeffs[i+1].setParameter(param);
			value = 2*A[i/2+1];
			vtdCoeffs[i+1].setValue(value);				
		}
	}
	// Other closure methods here (including HVVP)

// 	//GVTD-MeanWind closure:
// 	if(closure.contains(QString("MeanWind"),Qt::CaseInsensitive)) {
// 	  thetaT = atan2(yCenter,xCenter);
// 	  thetaT = fixAngle(thetaT);
// 	  float MeanWindDiff = 1;
// 	  float NewMeanWind = 0;
// 	  //find mean wind:
// 	  float VMx = (VdDx2-VdDx1)/(x2-x1);
// 	  float VMy = (VdDy2-VdDy1)/(y2-y1);
// 	  float GVTDMeanWind = ((VMx)**2 + (VMy)**2)**(1/2);
// 	  float thetaM = atan2(VMy,VMx);
// 	  thetaM = fixAngle(thetaM);
// 	  while (MeanWindDiff > .1) {

// 		GVTDMeanWind = ((VMx)**2 + (VMy)**2)**(1/2);
// 		thetaM = atan2(VMy,VMx);
// 		thetaM = fixAngle(thetaM);

// 		vtdCoeffs[0].setLevel(level);
// 		vtdCoeffs[0].setRadius(radius);
// 		vtdCoeffs[0].setParameter("VTC0");
// 		float value;
// 		value = -B[1]-B[3]-GVTDMeanWind*sin(thetaT-thetaM);
// 		vtdCoeffs[0].setValue(value);
		
// 		vtdCoeffs[1].setLevel(level);
// 		vtdCoeffs[1].setRadius(radius);
// 		vtdCoeffs[1].setParameter("VRC0");
// 		value = A[0]+A[1]+A[2]+A[3]+A[4]-GVTDMeanWind*sin(thetaT-thetaM);
// 		vtdCoeffs[1].setValue(value);

// 		vtdCoeffs[2].setLevel(level);
// 		vtdCoeffs[2].setRadius(radius);
// 		vtdCoeffs[2].setParameter("VMC0");
// 		value = A[0]+A[2]+A[4];
// 		vtdCoeffs[2].setValue(value);
		
// 		vtdCoeffs[3].setLevel(level);
// 		vtdCoeffs[3].setRadius(radius);
// 		vtdCoeffs[3].setParameter("VTS1");
// 		if ((sinAlphamax < 0.8) and (numCoeffs >= 5)) {
// 			value = A[2]-A[0]+A[4]+(A[0]+A[2]+A[4])*cosAlphamax;
// 			if (value < vtdCoeffs[0].getValue()) {
// 				vtdCoeffs[3].setValue(value);
// 			} else {
// 				vtdCoeffs[3].setValue(value);
// 			}
// 		} else {
// 			vtdCoeffs[3].setValue(0);
// 		}
		
// 		vtdCoeffs[4].setLevel(level);
// 		vtdCoeffs[4].setRadius(radius);
// 		vtdCoeffs[4].setParameter("VTC1");
// 		if ((sinAlphamax < 0.8) and (numCoeffs >= 5)) {
// 			value = -2.*(B[2]+B[4]);
// 			if (value < vtdCoeffs[0].getValue()) {
// 				vtdCoeffs[4].setValue(value);
// 			} else {
// 				vtdCoeffs[4].setValue(value);
// 			}
// 		} else {
// 			vtdCoeffs[4].setValue(0);
// 		}
// 		for (int i=5; i <= numCoeffs-1; i+=2) {
// 			vtdCoeffs[i].setLevel(level);
// 			vtdCoeffs[i].setRadius(radius);
// 			QString param = "VTC" + QString().setNum(int(i/2));
// 			vtdCoeffs[i].setParameter(param);
// 			value = -2*B[i/2+1];
// 			vtdCoeffs[i].setValue(value);	
			
// 			vtdCoeffs[i+1].setLevel(level);
// 			vtdCoeffs[i+1].setRadius(radius);
// 			param = "VTS" + QString().setNum(int(i/2));
// 			vtdCoeffs[i+1].setParameter(param);
// 			value = 2*A[i/2+1];
// 			vtdCoeffs[i+1].setValue(value);				
// 		}
// 		NewMeanWind = ???????;
// 		MeanWindDiff = abs(GVTDMeanWind-NewMeanWind);
// 		GVTDMeanWind = NewMeanWind;
// 	  }
// 	}
	
//	else {
	  // Use the GVTD closure method to set VT, VR, and VM		
	if (closure.contains(QString("gvtd"),Qt::CaseInsensitive)) {
	        float thetaT = atan2(ycenter,xcenter);
		thetaT = fixAngle(thetaT);
		
		//Message::toScreen(QString().setNum(radius)+" "+QString().setNum(centerDistance)+" "+QString().setNum(A[0])+" "+QString().setNum(A[1])+" "+QString().setNum(A[2])+" "+QString().setNum(A[3])+" "+QString().setNum(A[4]));

	        vtdCoeffs[0].setLevel(level);
		vtdCoeffs[0].setRadius(radius);
		vtdCoeffs[0].setParameter("VTC0");
		float value;
		float VTS1;
		float VRC0;

		//REMOVE/COMMENT THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//	float GVTDMeanWind = 5;
		//	float thetaM = 0;
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		if(closure.contains(QString("hvvp"),Qt::CaseInsensitive) and 
		   (B[1] != 0)) {
		  value = -B[1]-B[3]-(radius/centerDistance)*hvvpMeanWind*sin(thetaT-HVVPThetaM);
		}
		else {
		  value = -B[1]-B[3];
		}
		vtdCoeffs[0].setValue(value);
		
		vtdCoeffs[1].setLevel(level);
		vtdCoeffs[1].setRadius(radius);
		vtdCoeffs[1].setParameter("VRC0");
		value = (A[0]+A[1]+A[2]+A[3]+A[4])/(1 + radius/centerDistance);
		vtdCoeffs[1].setValue(value);
		VRC0 = value;
		
		vtdCoeffs[3].setLevel(level);
		vtdCoeffs[3].setRadius(radius);
		vtdCoeffs[3].setParameter("VTS1");
		if ((sinAlphamax < 0.8) and (numCoeffs >= 5)) {
			value = A[2]-A[0]+A[4]+(A[0]+A[2]+A[4])*cosAlphamax;
			VTS1 = value;
			if (value < vtdCoeffs[0].getValue()) {
				vtdCoeffs[3].setValue(value);
			} else {
				vtdCoeffs[3].setValue(value);
			}
		} else {
			vtdCoeffs[3].setValue(0);
			VTS1 = 0;
		}

		vtdCoeffs[2].setLevel(level);
		vtdCoeffs[2].setRadius(radius);
		vtdCoeffs[2].setParameter("VMC0");
		value = A[0] - (radius/centerDistance)*VRC0 + .5*VTS1;
		vtdCoeffs[2].setValue(value);
		
		vtdCoeffs[4].setLevel(level);
		vtdCoeffs[4].setRadius(radius);
		vtdCoeffs[4].setParameter("VTC1");
		if ((sinAlphamax < 0.8) and (numCoeffs >= 5)) {
			value = -2.*(B[2]+B[4]);
			if (value < vtdCoeffs[0].getValue()) {
				vtdCoeffs[4].setValue(value);
			} else {
				vtdCoeffs[4].setValue(value);
			}
		} else {
			vtdCoeffs[4].setValue(0);
		}
		for (int i=5; i <= numCoeffs-1; i+=2) {
			vtdCoeffs[i].setLevel(level);
			vtdCoeffs[i].setRadius(radius);
			QString param = "VTC" + QString().setNum(int(i/2));
			vtdCoeffs[i].setParameter(param);
			value = -2*B[i/2+1];
			vtdCoeffs[i].setValue(value);	
			
			vtdCoeffs[i+1].setLevel(level);
			vtdCoeffs[i+1].setRadius(radius);
			param = "VTS" + QString().setNum(int(i/2));
			vtdCoeffs[i+1].setParameter(param);
			value = 2*A[i/2+1];
			vtdCoeffs[i+1].setValue(value);				
		}
		
		
	}


	delete[] A;
	delete[] B;
	
}

float GBVTD::setMeanWindCoefficients(float& radius, float& level, int& numCoeffs, float*& FourierCoeffs, Coefficient*& vtdCoeffs, float xCenter, float yCenter)
{
	
        float VMCosCheck;

	// Allocate and initialize the A & B coefficient arrays
	float* A;
	float* B;
	int maxIndex = numCoeffs/2 + 1;
	if (maxIndex > 5) {		
		A = new float[numCoeffs/2 + 1];
		B = new float[numCoeffs/2 + 1];
	} else {
		A = new float[5];
		B = new float[5];
	}
	for (int i=0; i <= 4; i++) {
		A[i] = 0;
		B[i] = 0;
	}
		
	float sinAlphamax = radius/centerDistance;
	float cosAlphamax = sqrt(centerDistance * centerDistance - radius*radius)/centerDistance;
	A[0] = FourierCoeffs[0];
	B[0] = 0.;
	for (int i=1; i <= (numCoeffs/2); i++) {
		A[i] = FourierCoeffs[2*i];
		B[i] = FourierCoeffs[2*i-1];
	}
	
	// Use the specified closure method to set VT, VR, and VM		

	// Other closure methods here (including HVVP)

	//GVTD-MeanWind closure:
	if(closure.contains(QString("MeanWind"),Qt::CaseInsensitive)) {

	  thetaT = atan2(yCenter,xCenter);
	  thetaT = fixAngle(thetaT);
	  float thetaM = atan2(VMy,VMx);
	  thetaM = fixAngle(thetaM);
	  //  float MeanWindDiff = 100;
	  //  float NewMeanWind = 0;
	  //find mean wind:
// 	  float VMx = (VdDx2-VdDx1)/(x2-x1);
// 	  float VMy = (VdDy2-VdDy1)/(y2-y1);
	  float GVTDMeanWind = sqrt( VMx*VMx + VMy*VMy  );

	  //	  while (MeanWindDiff > 10) {

//	 	GVTDMeanWind = ((VMx)**2 + (VMy)**2)**(1/2);
// 		thetaM = atan2(VMy,VMx);
// 		thetaM = fixAngle(thetaM);



		//REMOVE/COMMENT THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//GVTDMeanWind = 5;
		//thetaM = 0;
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



		vtdCoeffs[0].setLevel(level);
		vtdCoeffs[0].setRadius(radius);
		vtdCoeffs[0].setParameter("VTC0");
		float value;
		float VTS1;
		float VRC0;
		float VMC0;
		value = -B[1]-B[3]-(radius/centerDistance)*GVTDMeanWind*sin(thetaT-thetaM);
		vtdCoeffs[0].setValue(value);
		
		vtdCoeffs[1].setLevel(level);
		vtdCoeffs[1].setRadius(radius);
		vtdCoeffs[1].setParameter("VRC0");
		value = (A[0]+A[1]+A[2]+A[3]+A[4])/(1 + radius/centerDistance)-GVTDMeanWind*cos(thetaT-thetaM);
		//value = (A[0]+A[1]+A[2]+A[3]+A[4])/(1 + radius/centerDistance);
		vtdCoeffs[1].setValue(value);
		VRC0 = value;
	
		vtdCoeffs[3].setLevel(level);
		vtdCoeffs[3].setRadius(radius);
		vtdCoeffs[3].setParameter("VTS1");
		if ((sinAlphamax < 0.8) and (numCoeffs >= 5)) {
			value = A[2]-A[0]+A[4]+(A[0]+A[2]+A[4])*cosAlphamax;
			VTS1 = value;
			if (value < vtdCoeffs[0].getValue()) {
				vtdCoeffs[3].setValue(value);
			} else {
				vtdCoeffs[3].setValue(value);
			}
		} else {
			vtdCoeffs[3].setValue(0);
			VTS1 = 0;
		}

		vtdCoeffs[2].setLevel(level);
		vtdCoeffs[2].setRadius(radius);
		vtdCoeffs[2].setParameter("VMC0");
		value = A[0] - (radius/centerDistance)*VRC0 + .5*VTS1;
		vtdCoeffs[2].setValue(value);
		VMC0 = value;
		
		vtdCoeffs[4].setLevel(level);
		vtdCoeffs[4].setRadius(radius);
		vtdCoeffs[4].setParameter("VTC1");
		if ((sinAlphamax < 0.8) and (numCoeffs >= 5)) {
			value = -2.*(B[2]+B[4]);
			if (value < vtdCoeffs[0].getValue()) {
				vtdCoeffs[4].setValue(value);
			} else {
				vtdCoeffs[4].setValue(value);
			}
		} else {
			vtdCoeffs[4].setValue(0);
		}
		for (int i=5; i <= numCoeffs-1; i+=2) {
			vtdCoeffs[i].setLevel(level);
			vtdCoeffs[i].setRadius(radius);
			QString param = "VTC" + QString().setNum(int(i/2));
			vtdCoeffs[i].setParameter(param);
			value = -2*B[i/2+1];
			vtdCoeffs[i].setValue(value);	
			
			vtdCoeffs[i+1].setLevel(level);
			vtdCoeffs[i+1].setRadius(radius);
			param = "VTS" + QString().setNum(int(i/2));
			vtdCoeffs[i+1].setParameter(param);
			value = 2*A[i/2+1];
			vtdCoeffs[i+1].setValue(value);				
		}
		VMCosCheck = VMC0;

		//}
	}	
	delete[] A;
	delete[] B;
	return VMCosCheck;
}

// int GBVTD::getNumCoefficients(int& numData,float& r, float& z)
// {

// 	int maxCoeffs = maxWavenumber*2 + 3;
// 	int numCoeffs = maxCoeffs;
	
// 	// Find the data gaps
// 	bool degreeSector[360];
// 	for (int i=0; i<=360; i++) degreeSector[i]=false;

// 	if (geometry == "GBVTD") {

// 	  for (int i=0; i<=numData-1; i++) {
// 	    int j = int(psi[i]*rad2deg);
// 	    if (j > 360) j = j - 360;
// 	    degreeSector[j] = true;
// 	  }
// 	}
// 	if (geometry == "GVTD") {

// 	  for (int i=0; i<=numData-1; i++) {
// 	    int j = int(thetaP[i]*rad2deg);
// 	    if (j > 360) j = j - 360;
// 	    degreeSector[j] = true;
// 	  }
// 	}
	
// 	// Check the width of the gap
// 	// Run completely around circle in case there is a gap at the beginning

// 	int gapSum = 0;

// 	for (int deg=0; deg<720; deg++) {
	 
// 		int j = deg%360;
// 		if (degreeSector[j]) {
// 			gapSum = 0;
// 			if (deg >= 360) {
// 				// We've come back around the circle, send back the current coefficient number
// 				if (r==79 && z == 9) {
// 				  for (int deg=0; deg<=360; deg++) {
// 				    Message::toScreen(QString().setNum(deg)+","+QString().setNum(degreeSector[deg]));
// 				  }
// 				}
// 				return numCoeffs;

// 			}
// 		} else {
// 			gapSum++;
// 			for (int i=maxWavenumber; i>=1; i--) {
// 				if (gapSum > dataGaps[i]) {
// 					// Gap is too large, reduce the number of coefficients
// 					numCoeffs = (i-1)*2 + 3;
// 				}
// 			}
// 			if (gapSum > dataGaps[0]) {
// 				// Can't even fit wavenumber zero
// 				if (r==79 && z == 9) {
// 				  for (int deg=0; deg<=360; deg++) {
// 				    Message::toScreen(QString().setNum(deg)+","+QString().setNum(degreeSector[deg]));
// 				  }
// 				}
// 				return 0;

// 			}
// 		}
// 	}
	
// 	// Shouldn't get here, if we do return 0
// 	return 0;
// }

int GBVTD::getNumCoefficients(int& numData)
{

	int maxCoeffs = maxWavenumber*2 + 3;
	int numCoeffs = maxCoeffs;
	
	// Find the data gaps
	bool degreeSector[360];
	for (int i=0; i<=360; i++) degreeSector[i]=false;

	if (geometry == "GBVTD") {

	  for (int i=0; i<=numData-1; i++) {
	    int j = int(psi[i]*rad2deg);
	    if (j > 360) j = j - 360;
	    degreeSector[j] = true;
	  }
	}
	if (geometry == "GVTD") {

	  for (int i=0; i<=numData-1; i++) {
	    int j = int(thetaP[i]*rad2deg);
	    if (j > 360) j = j - 360;
	    degreeSector[j] = true;
	  }
	}
	
	// Check the width of the gap
	// Run completely around circle in case there is a gap at the beginning

	int gapSum = 0;

	for (int deg=0; deg<720; deg++) {
	 
		int j = deg%360;
		if (degreeSector[j]) {
			gapSum = 0;
			if (deg >= 360) {
				// We've come back around the circle, send back the current coefficient number
				return numCoeffs;
			}
		} else {
			gapSum++;
			for (int i=maxWavenumber; i>=1; i--) {
				if (gapSum > dataGaps[i]) {
					// Gap is too large, reduce the number of coefficients
					numCoeffs = (i-1)*2 + 3;
				}
			}
			if (gapSum > dataGaps[0]) {
				// Can't even fit wavenumber zero
				return 0;
// 				for (int deg=0; deg<=360; deg++) {
// 				 Message::toScreen(QString().setNum(deg)+","+QString().setNum(degreeSector[deg])+"\n");
// 				}
			}
		}
	}
	
	// Shouldn't get here, if we do return 0
	return 0;
}

float GBVTD::fixAngle(float& angle)
{
	
	// Make sure an angle is between 0 and 2Pi
	if (fabs(angle) < 1.0e-06) angle = 0.0;
	if (angle > (2*Pi)) angle = angle - 2*Pi;
	if (angle < 0.) angle = angle + 2*Pi;
	return angle;

}


void GBVTD::setHVVP(const float& meanWind)
{
  hvvpMeanWind = meanWind;
}

void GBVTD::setHVVPangle(const float& angle)
{
  HVVPThetaM = angle;
}

void GBVTD::setGVTD(const float& X,const float& Y)
{
  VMx = X;
  VMy = Y;
}
