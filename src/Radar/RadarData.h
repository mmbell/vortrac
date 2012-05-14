/*
 *  RadarData.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef RADARDATA_H
#define RADARDATA_H

#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDomElement>
#include "Sweep.h"
#include "Ray.h"

class RadarData
{

public:

    RadarData(QString radarname, float lat, float lon, QString filename);
    virtual ~RadarData();
    virtual bool readVolume() = 0;
    Sweep* getSweep(int index);
    Ray* getRay(int index);
    int getNumRays();
    int getNumSweeps();
    bool isDealiased() { return dealiased; }
    void isDealiased(bool flag) { dealiased = flag; }
    QString getDateTimeString();
    QDateTime getDateTime();
    float* getRadarLat();
    float* getRadarLon();
    float radarBeamHeight(float &distance, float elevation);
    // returns height in km from radar;
    float absoluteRadarBeamHeight(float &distance, float elevation);
    // returns height in km from sea level;
    int getVCP() {return vcp;}
    void setAltitude(const float newAltitude);
    bool writeToFile(const QString fileName);
    bool fileIsReadable();
    QString getFileName();


protected:
    QString radarName;
    float radarLat;
    float radarLon;
    QDateTime radarDateTime;
    QString radarFileName;
    QFile* radarFile;
    Sweep* Sweeps;
    Ray* Rays;
    int numSweeps;
    int numRays;
    float Pi;
    float deg2rad;
    float rad2deg;
    int vcp;
    float altitude; // Tower height from sea level in km

private:
    bool dealiased;

};


#endif

