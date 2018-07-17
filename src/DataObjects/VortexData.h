/*
 *  VortexData.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 8/2/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef VORTEXDATA_H
#define VORTEXDATA_H

#include "Coefficient.h"
#include <QDateTime>

class VortexData
{

public:
    VortexData();
    VortexData(int availLevels, int availRadii, int availWaveNum);
    VortexData(const VortexData &other);
    ~VortexData();

    static constexpr float _fillv  =-999.0f;

    //functions about the center info of vortex
    inline float getLat(int index=0) const     { return (index < _numLevels) ? _centerLat[index] : _fillv;}
    inline void  setLat(int index,float lat)   { if(index<_numLevels) _centerLat[index]=lat;}
    inline void  setLat(float a[],int howMany) { for(int i=0;i<howMany;i++) setLat(i,a[i]);}

    inline float getLon(int index=0) const     { return (index<_numLevels)?_centerLon[index]:_fillv;}
    inline void  setLon(int index,float lon)   { if(index<_numLevels) _centerLon[index]=lon;}
    inline void  setLon(float a[],int howMany) { for(int i=0;i<howMany;i++) setLon(i,a[i]);}

    inline float getHeight(int index=0) const      { return (index<_numLevels)?_centerAlt[index]:_fillv;}
    inline void  setHeight(int index, float alt)   { if(index<_numLevels) _centerAlt[index]=alt;}
    inline void  setHeight(float a[],int howMany)  { for(int i=0;i<howMany;i++) setHeight(i,a[i]);}
    int getHeightIndex(const float& height) const;

    inline QDateTime getTime() const                {return _time;}
    inline void setTime(const QDateTime& radartime) { _time=radartime;}

    inline float getMaxVT(int index=0) const    { return (index<_numLevels)?_maxVT[index]:-999.0f; }
    inline void setMaxVT(int index,float value){ if(index<_numLevels) _maxVT[index]=value; }
    inline float getRMW(int index=0) const      { return (index<_numLevels)?_RMW[index]:_fillv;}
    inline float getRMWnm(int index=0) const      { return (index<_numLevels)?_RMW[index]*0.5399568:_fillv;}
    inline void  setRMW(int index,float value)  { if(index<_numLevels) _RMW[index]=value;}
    inline void  setRMW(float a[],int howMany)  { for(int i=0;i<howMany;i++) setRMW(i,a[i]);}
    inline float getRMWUncertainty(int index=0) const      { return (index<_numLevels)?_RMWUncertainty[index]:_fillv;}
    inline float getRMWUncertaintynm(int index=0) const      { return (index<_numLevels)?_RMWUncertainty[index]*0.5399568:_fillv;}
    inline void  setRMWUncertainty(int index,float value)  { if(index<_numLevels) _RMWUncertainty[index]=value;}
    inline void  setRMWUncertainty(float a[],int howMany)  { for(int i=0;i<howMany;i++) setRMW(i,a[i]);}
    inline float getMaxSfcWind() const    { return maxSfcWind; }
    inline void  setMaxSfcWind(float value){ maxSfcWind=value; }


    inline float getAveRMW() const      { return _aveRMW; }
    inline float getAveRMWnm() const      { return _aveRMW*0.5399568; }
    inline void  setAveRMW(float value) { _aveRMW=value; }
    inline float getAveRMWUncertainty() const      { return _aveRMWUncertainty; }
    inline float getAveRMWUncertaintynm() const      { return _aveRMWUncertainty*0.5399568; }
    inline void  setAveRMWUncertainty(float value) { _aveRMWUncertainty=value; }
    inline float getMaxValidRadius() const      { return _maxValidRadius; }
    inline float getMaxValidRadiusnm() const      { return _maxValidRadius*0.5399568; }
    inline void  setMaxValidRadius(float value) { _maxValidRadius=value; }

    inline float getPressure() const       { return centralPressure; }
    inline void  setPressure(float value)  { centralPressure=value; }
    inline float getPressureUncertainty() const      { return centralPressureUncertainty; }
    inline void  setPressureUncertainty(float value) { centralPressureUncertainty=value; }
    inline float getPressureDeficit() const       { return pressureDeficit; }
    inline void  setPressureDeficit(float value)  { pressureDeficit=value; }
    inline float getDeficitUncertainty() const       { return pressureDeficitUncertainty; }
    inline void  setDeficitUncertainty(float value)  { pressureDeficitUncertainty=value; }

    //inline int  getNumConvergingCenters(int index) const       { return (index<_numLevels)?numConvergingCenters[index]:int(_fillv); }
    //inline void setNumConvergingCenters(int index, int number) { if(index<_numLevels) numConvergingCenters[index]=number; }
    //inline void setNumConvergingCenters(int a[], int howMany)  { for(int i=0;i<howMany;i++) setNumConvergingCenters(i,a[i]); }

    inline float getCenterStdDev(int index) const       { return (index<_numLevels)?_centerSD[index]:_fillv; }
    inline void  setCenterStdDev(int index,float value) { if(index<_numLevels) _centerSD[index]=value; }
    inline void  setCenterStdDev(float a[],int howMany) { for(int i=0;i<howMany;i++) setCenterStdDev(i,a[i]); }

    Coefficient getCoefficient(const int& lev, const int& rad,const int& waveNum) const;
    Coefficient getCoefficient(const int& lev, const int& rad,const QString& parameter) const;
    Coefficient getCoefficient(const float& height, const int& rad,const QString& parameter) const;
    Coefficient getCoefficient(const float& height, const float& rad,const QString& parameter) const;
    void	setCoefficient(const int& lev, const int& rad,const int& coeffNum, const Coefficient &coefficient);
    void	saveCoefficients(QString &fname);

    // void operator = (const VortexData &other);
    bool operator ==(const VortexData &other);
    bool operator < (const VortexData &other);
    bool operator > (const VortexData &other);

    inline int getNumLevels() const  { return _numLevels; }
    inline int getNumRadii() const   { return _numRadii; }
    inline int getNumWaveNum() const { return _numWaveNum; }

    inline void setNumLevels(const int& num)  { if(num <= MAXLEVELS)  _numLevels = num; }
    inline void setNumRadii(const int& num)   { if(num <= MAXRADII)   _numRadii = num; }
    inline void setNumWaveNum(const int& num) { if(num <= MAXWAVENUM) _numWaveNum = num; }

    static int getMaxLevels()  { return MAXLEVELS; }
    static int getMaxRadii()   { return MAXRADII; }
    static int getMaxWaveNum() { return MAXWAVENUM; }

    int        getBestLevel()	const	{ return _bestLevel; }
    inline void setBestLevel(int l)	{ _bestLevel = l; }
    
private:
    // static const int MAXLEVELS  = 15;
    static const int MAXLEVELS  = 25;
    static const int MAXRADII   = 151;
    static const int MAXWAVENUM = 7;  // was 5

    int _numLevels;
    int _numRadii;
    int _numWaveNum;
    int _bestLevel;
    
    float _centerLat[MAXLEVELS];
    float _centerLon[MAXLEVELS];
    float _centerAlt[MAXLEVELS];
    float _maxVT[MAXLEVELS];
    float _RMW[MAXLEVELS];
    float _RMWUncertainty[MAXLEVELS];
    float _centerSD[MAXLEVELS];
    Coefficient coefficients[MAXLEVELS][MAXRADII][MAXWAVENUM*2+3];

    QDateTime _time;
    float _maxValidRadius;
    float _aveRMW;
    float _aveRMWUncertainty;
    float centralPressure;
    float centralPressureUncertainty;
    float pressureDeficit;
    float pressureDeficitUncertainty;
    float maxSfcWind;

};


#endif


