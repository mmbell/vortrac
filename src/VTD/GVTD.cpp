/*
 *  GVTD.cpp
 *  vortrac
 *
 *  Copyright 2017 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <cstdlib>
#include <fstream>

#include "GVTD.h"
#include <math.h>
#include "IO/Message.h"
#include "Math/Matrix.h"


GVTD::GVTD(QString& initClosure, int& wavenumbers, float*& gaps, float hvvpwind)
  : VTD(initClosure, wavenumbers, gaps, hvvpwind)
{
}

GVTD::~GVTD()
{
}

bool GVTD::analyzeRing(float& xCenter, float& yCenter, float& radius, float& height, int& numData, 
                        float*& ringData, float*& ringAzimuths, Coefficient*& vtdCoeffs, float& vtdStdDev)
{
  // Implement GVTD by Ting-Yu Cha, 11/03/2017
  // Analye a ring of data

  // Make a Psi array
  
  ringPsi = new float[numData];
  vel = new float[numData];
  psi = new float[numData];
  float *ringDistance = new float[numData];

  // Get thetaT
  thetaT = atan2(yCenter,xCenter);
  thetaT = fixAngle(thetaT);
  centerDistance = sqrt(xCenter * xCenter + yCenter * yCenter);

  for (int i = 0; i < numData; i++) {
    // Convert to Psi
    float angle = ringAzimuths[i] * DEG2RAD - thetaT;
    angle = fixAngle(angle);
    float xx = xCenter + radius * cos(angle + thetaT);
    float yy = yCenter + radius * sin(angle + thetaT);
    ringDistance[i] = sqrt(xx * xx + yy * yy);
    ringPsi[i] = angle;
  }

  // Threshold bad values
  int goodCount = 0;

  for (int i = 0; i < numData; i++) {
    if (ringData[i] != -999.) {
      // Good point
      vel[goodCount] = ringData[i] * ringDistance[i] / centerDistance;
      psi[goodCount] = ringPsi[i];
      goodCount++;
    }
  }
  numData = goodCount;

  // Get the maximum number of coefficients for the given data distribution and geometry
  int numCoeffs = getNumCoefficients(numData);

  if (numCoeffs == 0) {
    // Too much missing data, set everything to 0 and return
    for (int i = 0; i <= (_maxWaveNum * 2 + 2); i++) {
      FourierCoeffs[i] = 0.;
    }
    vtdStdDev = -999;
    setWindCoefficients(radius, height, numCoeffs, FourierCoeffs, vtdCoeffs);
    delete[] ringPsi;
    delete[] vel;
    delete[] psi;
    delete[] ringDistance;
    return false;
  }

  // Least squares
  
  float** xLLS = new float*[numCoeffs];
  for (int i = 0; i <= numCoeffs - 1; i++) {
    xLLS[i] = new float[numData];
  }
  float* yLLS = new float[numData];
  for (int i = 0; i <= numData - 1; i++) {
    xLLS[0][i] = 1.;
    for (int j = 1; j <= (numCoeffs / 2); j++) {
      xLLS[2 * j - 1][i] = sin(float(j) * psi[i]);
      xLLS[ 2 * j][i] = cos(float(j) * psi[i]);
    }
    yLLS[i] = vel[i];
  }

  float* stdError = new float[numCoeffs];
  if( ! Matrix::lls(numCoeffs, numData, xLLS, yLLS, vtdStdDev, FourierCoeffs, stdError)) {
    for (int i = 0; i <= numCoeffs - 1; i++)
      delete[] xLLS[i];
    delete[] yLLS;
    delete[] stdError;
    delete[] ringPsi;
    delete[] vel;
    delete[] psi;
    return false;
  }

  // Convert Fourier coefficients into wind coefficients
  setWindCoefficients(radius, height, numCoeffs, FourierCoeffs, vtdCoeffs);
  
  for (int i = 0; i <= numCoeffs - 1; i++)
    delete[] xLLS[i];
  
  delete[] xLLS;
  delete[] yLLS;
  delete[] stdError;
  delete[] ringPsi;
  delete[] vel;
  delete[] psi;

  return true;
}

void GVTD::setWindCoefficients(float& radius, float& level, int& numCoeffs, float*& FourierCoeffs,
				Coefficient*& vtdCoeffs)
{
    // Allocate and initialize the A & B coefficient arrays
  
    float* A;
    float* B;
    int maxIndex = numCoeffs / 2 + 1;
    
    if (maxIndex > 5) {
        A = new float[numCoeffs / 2 + 1];
        B = new float[numCoeffs / 2 + 1];
    } else {
        A = new float[5];
        B = new float[5];
    }
    for (int i=0; i <= 4; i++) {
        A[i] = 0;
        B[i] = 0;
    }

    // float sinAlphamax = radius/centerDistance;
    // float cosAlphamax = sqrt(centerDistance * centerDistance - radius * radius) / centerDistance;
    
    A[0] = FourierCoeffs[0];
    B[0] = 0.;

    int max = numCoeffs / 2;

    for (int i=1; i <= max; i++) {
        A[i] = FourierCoeffs[2 * i];
        B[i] = FourierCoeffs[2 * i - 1];
    }

    // If environment variable is set, dump coeffs for Ting-Yu

    std::ofstream ofs;

    if (const char *env_v1 = std::getenv("VORTRAC_DUMP_COEFFS") ) {
      ofs.open(env_v1, std::ofstream::out | std::ofstream::app);

      if (ofs.good() ) {
	ofs << "# radius: " << radius << ", level: " << level << ", numCoeffs: " << numCoeffs
	    << " A[0.." << max << "], B[0.." << max << "]"
	    << std::endl;
	
	for(int i = 0; i <= max; i++)
	  ofs << A[i] << ", ";
	for(int i = 0; i <= max; i++)
	  ofs << B[i] << ", ";
	ofs << std::endl;
      } else {
	std::cerr << "Couldn't open " << env_v1 << std::endl;
      }
    }
    
    // Use the specified closure method to set VT, VR, and VM
    if (closure.contains(QString("original"), Qt::CaseInsensitive)) {

      // Implement GVTD by Ting-Yu Cha 11/03/2017
      vtdCoeffs[0].setLevel(level);
      vtdCoeffs[0].setRadius(radius);
      vtdCoeffs[0].setParameter("VTC0");
      float value;
      value = - B[1] - B[3];
      vtdCoeffs[0].setValue(value);

      vtdCoeffs[1].setLevel(level);
      vtdCoeffs[1].setRadius(radius);
      vtdCoeffs[1].setParameter("VRC0");
      value = (A[0] + A[1] + A[2] + A[3] + A[4]) / ( 1 + radius / centerDistance);
      vtdCoeffs[1].setValue(value);

      // Set a default value in case it doesn't get set in the loop
      vtdCoeffs[4].setValue(0);
      
      for (int i=3; i <= numCoeffs - 1; i += 2) {
	vtdCoeffs[i].setLevel(level);
	vtdCoeffs[i].setRadius(radius);
	QString param = "VTC" + QString().setNum(int(i / 2));
	vtdCoeffs[i].setParameter(param);
	value = -2. * B[i / 2 + 1];
	vtdCoeffs[i].setValue(value);

	vtdCoeffs[i+1].setLevel(level);
	vtdCoeffs[i+1].setRadius(radius);
	param = "VTS" + QString().setNum(int(i / 2));
	vtdCoeffs[i + 1].setParameter(param);
	value = 2 * A[i / 2 + 1];
	vtdCoeffs[i + 1].setValue(value);
      }
      
      vtdCoeffs[2].setLevel(level);
      vtdCoeffs[2].setRadius(radius);
      vtdCoeffs[2].setParameter("VMC0");
      value = A[0] - ( radius / centerDistance * vtdCoeffs[1].getValue() ) + 0.5 * vtdCoeffs[4].getValue();
      // rhs value is VRC0 value computed just above
      vtdCoeffs[2].setValue(value);
    }

    delete[] A;
    delete[] B;
}
