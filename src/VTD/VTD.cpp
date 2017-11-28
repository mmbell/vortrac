/*
 *  GBVTD.cpp
 *  vortrac
 *
 *  Created by Michael Bell on 5/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "VTD.h"
#include "GBVTD.h"
#include "GVTD.h"

#include <math.h>
#include "IO/Message.h"
#include "Math/Matrix.h"

const float VTD::PI      = 3.1415926f;
const float VTD::DEG2RAD = PI/180.f;
const float VTD::RAD2DEG = 180.f/PI;

VTD::VTD(QString& initClosure, int& wavenumbers, float*& gaps, float hvvpwind)
{
    closure = initClosure;
    _maxWaveNum = wavenumbers;
    dataGaps = gaps;
    FourierCoeffs = new float[_maxWaveNum * 2 + 3];
    _hvvpMean = hvvpwind;
}

VTD::~VTD()
{
    // Default destructor
    delete[] FourierCoeffs;
}

int VTD::getNumCoefficients(int& numData)
{
    int maxCoeffs = _maxWaveNum*2 + 3;
    int numCoeffs = maxCoeffs;

    // Find the data gaps
    bool degreeSector[360];
    for (int i=0; i<360; i++) degreeSector[i]=false;

    for (int i=0; i<=numData-1; i++) {
        int j = int(psi[i]*RAD2DEG);
        if (j > 359) j = j - 360;
        degreeSector[j] = true;
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
            for (int i=_maxWaveNum; i>=1; i--) {
                if (gapSum > dataGaps[i]) {
                    // Gap is too large, reduce the number of coefficients
                    numCoeffs = (i-1)*2 + 3;
                }
            }
            if (gapSum > dataGaps[0]) {
                // Can't even fit wavenumber zero
                return 0;
            }
        }
    }

    // Shouldn't get here, if we do return 0
    return 0;
}

float VTD::fixAngle(float& angle)
{
    // Make sure an angle is between 0 and 2Pi
  
    if (fabs(angle) < 1.0e-06) angle = 0.0;
    if (angle > (2*PI)) angle = angle - 2*PI;
    if (angle < 0.) angle = angle + 2*PI;
    return angle;

}

void VTD::setHVVP(const float& meanWind)
{
    _hvvpMean = meanWind;
}
