/*
 *  Hvvp.h
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 5/18/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef HVVP_H
#define HVVP_H

#include "RadarData.h"
#include "Message.h"
#include "Configuration.h"


class Hvvp : public QObject
{ 

 Q_OBJECT

public:
  
   Hvvp();    
   ~Hvvp();

   void setRadarData(RadarData *newVolume, float range, float angle, 
		     float vortexRmw);
   void setConfig(Configuration* newConfig);

   bool findHVVPWinds(bool both);

   float* getHeights() { return z; }
   float* getUWinds() { return u; }
   float* getVWinds() { return v; }
   float* getAcrossBeamWinds() { return vm_sin; }
   float getAvAcrossBeamWinds() { return av_VmSin; }
   float getAvAcrossBeamWindsStdError() { return stdErr_VmSin; }
   void setPrintOutput(const bool printToLog);
   
public slots:
   void catchLog(const Message& message);

private:
   RadarData *volume;
   Configuration* configData;
   int levels;
   float hgtStart;
   float hInc;
   float xt_threshold;
   
   /*
    * levels: The number of elevation tilts available including the 2
    *         split level tilts of VCP 121
    *
    */
   
   float velNull, cca, rt, rmw, rot;
   /*
    * rt: Range from radar to circulation center (km).
    *
    * cca: Meteorological azimuth angle of the direction
    *      to the circulation center (degrees from north).
    *
    * rmw: radius of maximum wind measured from the TC circulation
    *      center outward.
    *
    * rot: cca in radians.
    *
    */

   float deg2rad, rad2deg;

   float** xls;
   int xlsDimension;
   long maxpoints;
   float* yls;
   float* wgt;

   float *z, *u, *v, *vm_sin, *var, av_VmSin, stdErr_VmSin;
   /*
    * z: Array containing the 14 altitudes (km) AGL of HVVP analysis heights.
    *    Array dimensions: z(14).
    *
    * u: Array containing the U (m/s) coordinates of the environmental wind
    *    at each z.  Array dimensions: u(14).
    *
    * v: Array containing the V (m/s) coordinates of the environmental wind
    *    at each z.  Array dimensions: v(14).
    *
    * vm_sin: Array containing the across beam component of the environmental 
    *         wind, vm_sin (m/s), at each z.  Array dimensions: vm_sin(14).
    *
    * av_VmSin: USE THIS RESULT TO CORRECT VTC0 IN GBVTD:
    *
    *           The layer, variance-weighted average of the across beam 
    *           component of the environmental wind.  The mean height of the
    *           layer is 1.3 km if all 14 analysis altitudes contained enough
    *           data for 14 corresponding HVVP results. Even if not all 14 
    *           altitudes have HVVP results, IT IS STRONGLY RECOMMENDED that
    *           av_VmSin be used to correct VTC0 in the GBVTD results, not
    *           a specific vm_sin result in the array vm_sin(14) for a
    *           particular altitude.  The main reason is that if one picks
    *           a vm_sin value for use in a specific GBVTD CAPPI height it could
    *           result in a poor result if the standard error is large
    *           (as I have confirmed with the H. Katrina (2005) and Bret (1999)
    *           HVVP results - standard errors over 4 m/s were found in some
    *           cases). Since to calculate the error in the pressure deficit
    *           involves addition of errors (integration) it would be wise to
    *           keep the standard error in VmSin to a minimum. Using av_VmSin
    *           means that you are factoring in the reliability of 
    *           the data since the square of the standard error (variance) 
    *           is used to calculate this weighted average (Very encouraging
    *           results so far returned for H. Bret (1999), Charely (2004) and
    *           Katrina (2005)). 
    *
    * stdErr_VmSin: 
    *           The standard error in av_VmSin
    *
    */

   bool printOutput;

   /*
    * printOutput: If this is set to true the HVVP output will be printed to 
    *              the log file and message screen
    *
    */

   float rotateAzimuth(const float &angle);

   long hvvpPrep(int m);
   
   
   //Moved to static functions in Math/Matrix 
   bool lls(int numCoeff, int numData, int effective_nData, float** x, 
	    float* y, float* weight, float &stDeviation, float* &stError, 
	    float* &coEff);
     
   bool gaussJordan(float **a, float **b, int n, int m);
   
     
   void smoothHvvp(float* data);
   void smoothHvvpVmSin(float* data1, float* data2);
   void writeToFile(QString& nameOfFile, long aRows, long aCols,
		    long bRows, long bCols, float** a, float* b);
   void writeToFileWithAltitude(QString& nameOfFile, long aRows, long aCols,
		    long bRows, long bCols, float** a, float* b);

signals:
   void log(const Message& message);

};


#endif
