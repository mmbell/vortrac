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


class Hvvp : public QObject
{ 

 Q_OBJECT

public:
  
   Hvvp();    
   ~Hvvp();

   void setRadarData(RadarData *newVolume, float range, float angle, 
		     float vortexRmw);

   void findHVVPWinds();

   float* getHeights() { return z; }
   float* getUWinds() { return u; }
   float* getVWinds() { return v; }
   float* getAcrossBeamWinds() { return vm_sin; }
   float getAvAcrossBeamWinds() { return av_VmSin; }
   
public slots:
   void catchLog(const Message& message);

private:
   RadarData *volume;
   float velNull, cca, rt, rmw;
   /*
    * rt: Range from radar to circulation center (km).
    *
    * cca: Meteorological azimuth angle of the direction
    *      to the circulation center (degrees from north).
    *
    * rmw: radius of maximum wind measured from the TC circulation
    *      center outward.
    *
    */

   float deg2rad, rad2deg;

   float** xls;
   float* yls;
   float* wgt;

   float *z, *u, *v, *vm_sin, *var, av_VmSin;
   /*
    * z: Array containing the 12 altitudes (km) AGL of HVVP analysis heights.
    *    Array dimensions: z(12).
    *
    * u: Array containing the U (m/s) coordinates of the environmental wind
    *    at each z.  Array dimensions: u(12).
    *
    * v: Array containing the V (m/s) coordinates of the environmental wind
    *    at each z.  Array dimensions: v(12).
    *
    * vm_sin: Array containing the across beam component of the environmental 
    *         wind, vm_sin (m/s), at each z.  Array dimensions: vm_sin(12).
    *
    * av_VmSin: USE THIS RESULT TO CORRECT VTC0 IN GBVTD:
    *           The layer, variance-weighted average of the across beam 
    *           component of the environmental wind.  The mean height of the
    *           layer is 1.9 km if all 12 analysis altitudes contained enough
    *           data for 12 corresponding HVVP results. Even if not all 12 
    *           altitudes have HVVP results, IT IS STRONGLY RECOMMENDED that
    *           av_VmSin be used to correct VTC0 in the GBVTD results, not
    *           a specific vm_sin result in the array vm_sin(12) for a
    *           particular altitude.  The main reason is that 
    *           the array vm_sin(12) does not show the standard error
    *           of each vm_sin value at each altitude. To blindly pick
    *           a vm_sin value for use in a specific GBVTD CAPPI could
    *           result in a poor result if the standard error is large.
    *           However, to use av_VmSin instead means that you are
    *           factoring in the reliability of the data since the square
    *           of the standard error (variance) is used to calculate this
    *           weighted average.  Also, VAD-like (i.e., including VVP) 
    *           results in hurricanes have been shown to have significant 
    *           variance for individual wind results as a function of
    *           altitude (e.g. Marks VAD work with H. Fran).
    *           Thus, a layer average is a more precise result to use.
    *           Vm_sin should not vary a great deal anyway over the lowest
    *           3 km of atmosphere, so the use of an average as a proxy for
    *           any specific altitude is a reasonable thing to do. 
    *
    */

   float rotateAzimuth(const float &angle);

   int hvvpPrep(int m);

   bool lls(int numCoeff, int numData, int effective_nData, float** x, 
	    float* y, float* weight, float stDeviation, float* stError, 
	    float* coEff);

   bool gaussJordan(float **a, float **b, int n, int m);

signals:
   void log(const Message& message);

};


#endif
