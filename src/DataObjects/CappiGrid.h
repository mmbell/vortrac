/*
 *  CappiGrid.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/29/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef CAPPIGRID_H
#define CAPPIGRID_H

#include <QDomElement>
#include <QFile>
#include "Radar/RadarData.h"
#include "DataObjects/GriddedData.h"

class CappiGrid : public GriddedData
{

public:
    CappiGrid();
    ~CappiGrid();
    void  gridRadarData(RadarData *radarData, QDomElement cappiConfig,float *vortexLat, float *vortexLon);
    void  CressmanInterpolation(RadarData *radarData);
    float trilinear(const float &x, const float &y,const float &z, const int &param);
    void  writeAsi();
    bool  writeAsi(const QString& fileName);

private:

    float latReference;
    float lonReference;

    QString outFileName;
    float* relDist;

    class goodRef {
    public:
        float sumRef;
        float weight;
    };
    class goodVel {
    public:
        float sumVel;
        float height;
        float weight;
    };

    bool gridReflectivity;
    long maxRefIndex;
    long maxVelIndex;
    goodRef refValues[maxIDim][maxJDim][maxKDim];
    goodVel velValues[maxIDim][maxJDim][maxKDim];

};


#endif
