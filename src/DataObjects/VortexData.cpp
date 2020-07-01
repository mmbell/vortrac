/*
 *  VortexData.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 8/02/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <fstream>

#include "VortexData.h"
#include <QTextStream>
#include "IO/Message.h"
#include <math.h>

constexpr float VortexData::_fillv;

VortexData::VortexData()
{
    _numLevels = MAXLEVELS;
    _numRadii = MAXRADII;
    _numWaveNum = MAXWAVENUM;
    _bestLevel = -1;
    
    for(int i = 0; i < _numLevels; i++)
    {
        _centerLat[i] = -999;
        _centerLon[i] = -999;
        _centerAlt[i] = -999;
        _RMW[i] = -999;
        _RMWUncertainty[i] = -999;
        _centerSD[i] = -999;
    }

    _time = QDateTime();

    centralPressure = -999;
    centralPressureUncertainty = -999;
    pressureDeficit = -999;
    pressureDeficitUncertainty = -999;
	maxSfcWind = -999;
    _aveRMW = -999.0;
    _aveRMWUncertainty = -999.0;
    _maxValidRadius = -999;
}

VortexData::VortexData(int availLevels, int availRadii, int availWaveNum)
{
    _numLevels = availLevels;
    _numRadii = availRadii;
    _numWaveNum = availWaveNum;
    _bestLevel = -1;
    
    for(int i = 0; i < _numLevels; i++)
    {
        _centerLat[i] = -999;
        _centerLon[i] = -999;
        _centerAlt[i] = -999;
        _RMW[i] = -999;
        _RMWUncertainty[i] = -999;
        _centerSD[i] = -999;
    }

    _time = QDateTime();

    centralPressure = -999;
    centralPressureUncertainty = -999;
    pressureDeficit = -999;
    pressureDeficitUncertainty = -999;
    maxSfcWind = -999;
    _aveRMW = -999.0;
    _aveRMWUncertainty = -999.0;
    _maxValidRadius = -999;
}

VortexData::VortexData(const VortexData &other)
{
    this->_numLevels = other._numLevels;
    this->_numRadii  = other._numRadii;
    this->_numWaveNum = other._numWaveNum;
    this->_bestLevel = other._bestLevel;
    
    for(int i = 0; i < _numLevels; i++)
    {
        this->_centerLat[i] = other._centerLat[i];
        this->_centerLon[i] = other._centerLon[i];
        this->_centerAlt[i] = other._centerAlt[i];
        this->_maxVT[i] = other._maxVT[i];
        this->_RMW[i] = other._RMW[i];
        this->_RMWUncertainty[i] = other._RMWUncertainty[i];
        this->_centerSD[i] = other._centerSD[i];
        for(int j = 0; j < _numRadii; j++) {
            for(int k = 0; k < _numWaveNum; k++) {
                this->coefficients[i][j][k] = coefficients[i][j][k];
            }
        }
    }

    this->_time = other._time;

    this->centralPressure = other.centralPressure;
    this->centralPressureUncertainty = other.centralPressureUncertainty;
    this->pressureDeficit = other.pressureDeficit;
    this->pressureDeficitUncertainty = other.pressureDeficitUncertainty;
	this->maxSfcWind = other.maxSfcWind;
    this->_aveRMW = other._aveRMW;
    this->_aveRMWUncertainty = other._aveRMWUncertainty;
    this->_maxValidRadius = other._maxValidRadius;
}

VortexData::~VortexData()
{
}

// TODO
// This would be a lot simpler if the VortexData had access to minlevel, maxlevel, and grid spacing...

int VortexData::getHeightIndex(const float& height) const
{
    // Takes height in KM
    if(_centerAlt[0]==-999) {
        Message::toScreen("VORTEXDATA:NO HEIGHT DATA IS IN SYSTEM!!!!");
        return -1;
    }

    float heightDiff = 100;
    int closestIndex = -1;

    for(int i = 0; i < _numLevels; i++) {   // TODO involve firstLevel and lastLevel?
        if(_centerAlt[i] == -999) { continue; }
        if(fabs(height - _centerAlt[i]) < heightDiff) {
            heightDiff = fabs(height-_centerAlt[i]);
            closestIndex = i;
        }
    }
    if(closestIndex == -1) {
        Message::toScreen("VORTEXDATA:UNABLE TO FIND SUITABLE INDEX");
        return -1;
    }
    // std::cout << "getHeightIndex: height: " << height << ", index: " << closestIndex << std::endl;
    return closestIndex;
}

Coefficient VortexData::getCoefficient(const int& lev, const int& rad, 
                                       const int& waveNum) const
{
    return coefficients[lev][rad][waveNum];
}

Coefficient VortexData::getCoefficient(const int& lev, const int& rad,
                                       const QString& parameter) const
{
    for(int i = 0; i < _numWaveNum; i++) {
        if(coefficients[lev][rad][i].getParameter() == parameter)
            return coefficients[lev][rad][i];
    }
    return Coefficient();
}

Coefficient VortexData::getCoefficient(const float& height, const int& rad,
                                       const QString& parameter) const
{
    int level = getHeightIndex(height);
    if((level == -1) || (rad == -1)) {
        Message::toScreen("VortexData: GetCoefficient(3): Can't Get Needed Indices");
        return Coefficient();
    }
    return getCoefficient(level, rad, parameter);
}

Coefficient VortexData::getCoefficient(const float& height, const float& rad,
                                       const QString& parameter) const
{
    int level = getHeightIndex(height);
    if (level < 0) return Coefficient();
    float minRad = getCoefficient(level, 0, parameter).getRadius();
    if(minRad == -999)
        minRad = 0;
    int radIndex = int(rad - minRad);
    if((level == -1) || (radIndex < 0 ) || (radIndex > _numRadii)) {
        //Message::toScreen("VortexData: GetCoefficient(4): Can't Get Needed Indices: Level = "+QString().setNum(level)+" radIndex = "+QString().setNum(radIndex));
        return Coefficient();
    }
    return getCoefficient(level, radIndex, parameter);
}

void VortexData::setCoefficient(const int& lev, const int& rad, 
                                const int& coeffNum, const Coefficient &coefficient)
{
    coefficients[lev][rad][coeffNum] = coefficient;
}

bool VortexData::operator ==(const VortexData &other)
{
    if(this->_time == other._time)
        return true;
    return false;
}

bool VortexData::operator < (const VortexData &other)
{
    if(this->_time < other._time)
        return true;
    return false;
}

bool VortexData::operator > (const VortexData &other)
{
    if(this->_time > other._time)
        return true;
    return false;
}

// Append the vortex coefficients to a file

void VortexData::saveCoefficients(QString &fname)
{
  std::ofstream outfile(fname.toLatin1().data(), std::ios_base::app);

  outfile << "# Vortex time: " << getTime().toString("yyyy-MM-dd:hh:mm").toLatin1().data() << std::endl;

    for(int lev = 0; lev < _numLevels; lev++)
      for(int rad = 0; rad < _numRadii; rad++)
	for(int wave = 0; wave < _numWaveNum; wave++) {
	  Coefficient current = getCoefficient(lev, rad, wave);
	  if(current.getValue() <= _fillv)
	    continue;
	  outfile  << current.getLevel()
		   << "," << current.getRadius()
		   << "," << current.getParameter().toLatin1().data()
		   << "," << current.getValue()
		   << std::endl;
	}
    // file closed by the destructor.
}
