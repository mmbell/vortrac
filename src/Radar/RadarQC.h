/*
 *  RadarQC.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef RADARQC_H
#define RADARQC_H

#include "RadarData.h"
#include "Message.h"
#include <QWidget>
#include <QDomElement>
#include <QObject>
#include "Math/Matrix.h"

class RadarQC : public QObject
{ 
    Q_OBJECT

public:
    RadarQC(RadarData *radarPtr = 0, QObject *parent = 0);
    ~RadarQC();

    RadarData* getRadarData() {return radarData;}
    void getConfig(QDomElement qcConfig);
    /*
   * Retreves user parameters from the XML configuration file
   */

    bool dealias();
    /* Dealais runs all the seperate methods contained within this class
   * on a single radar volume. This includes: removal of the terminal
   * velocity component, basics thresholding with user adjusted parameters,
   * various methods to find environmental wind, and BB dealiasing
   */

    void debugDump(RadarData *radarPtr, int sweepNum);
    void dumpRay(RadarData *radarPtr, int rayNum);
    
private:

    RadarData *radarData;
    // Volume of Radar Data to be dealiased

    float specWidthLimit, velMin, velMax, refMin, refMax;
    /*
   * All these parameters are user adjustable for thresholding the
   *  data used in the analysis process
   *
   * specWidthLimit: specifies the larges spectral width a gate can
   *   have before it is eliminated from the radar volume.
   *
   * velMin: gates with velocities lower than velMin will be removed
   *   from the radar volume (km/s). (attempt to remove ground clutter)
   *
   * velMax: gates with velocities greater than velMax will be removed
   *   from the radar volume (km/s). (attempt to remove erronious data)
   *
   * refMin: gates with reflectivities lower than refMin will be removed
   *   from the radar volume (dBz). (attempt to remove erronious data)
   *
   * refMax: gate with reflectivities higher than refMax will be removed
   *   from the radar volume (dBz). (attempt to remove erronious data).
   *
   */


    float *envWind;
    float *envDir;
    /* envWind: Speed of mean environmental wind
   * envDir: Direction of the mean environmental wind in meteorological
   *   coordinates (degrees clockwise from North) and represents the direction
   *   the wind is coming from.
   * These are either determined by VAD algorithms, user input,
   *  or related AWIPS data (AWIPS not Yet Implemented)
   *
   */

    int maxFold;
    int numVGatesAveraged;
    /*
   * maxFold: the maximum number of allowable velocity folds the BB algorithm
   *   will attempt before giving an error.
   *
   * numVGatesAvereaged: the number of gates which are averaged together for
   *   the BB dealiasing routine.
   *
   */

    int vadLevels;
    int numCoEff;
    /*
   * vadLevels: The number of levels that VAD or GVAD should attempt to find
   *   the environmental wind for. The levels are spaced in km.
   *
   * numCoEff: The number of coefficents that should be returned from the
   *   least squares fit done within the VAD or GVAD algorithms.
   *
   */

    //QString AWIPSDir;
    /*
   * AWIPSDir: Location of relavent AWIPS data to be used in determining mean
   *   environmental winds.
   *
   */

    QString wind_method;
    /*
   * wind_method: Parameter indicating which method will be used to
   *   determine environmenal winds. Current options: VAD & GVAD, user, or
   *   AWIPS.
   *
   */

    bool useVADWinds;
    bool useGVADWinds;
    bool useUserWinds;
    //bool useAWIPSWinds;
    /*
   * These are boolean indicators taylor the program flow in accordance with
   *   which environmental retreival method was selected by the user.
   *
   */


    float deg2rad;           // Degrees to radians conversion factor
    float velNull;           // Value that indicates a null velocity value
    int q, qinc;             //  ????????????????????
    float radarHeight;       // Absolute height of radar in km from sea leve


    void thresholdData();
    /*
   * Primary Quality Control Method
   *   Eliminates velocity value within the data that do not meet the user
   *   specified values for velocity, reflectivity and spectral width.
   *
   */

    bool terminalVelocity();
    /*
   * Uses reflectivity data to approximate the terminal velocity component
   *   for each gate, these values are subtracted from the doppler velocity
   *   readings in the radar volume.
   *
   */

    bool findEnvironmentalWind();
    /*
   * Provides environmental wind according to user specified methods
   *
   */

    bool findVADStart(bool useGVAD);
    /*
   * This method is used to run either the VAD or GVAD algorithms including
   *   any intiailization or preparation algorithms, ie vadPrep.
   *
   */

    void vadPrep();
    /*
   * This method is used to initialize many VAD variables.
   *
   */

    bool VAD(float* &vel, Sweep* &currentSweep,
             float &speed, float &direction, float &rms);
    bool GVAD(float* &vel, Sweep* &currentSweep,
              float &speed, float &direction, float &rms);
    /*
   * These methods determine the environmental wind based on a least squares
   *   fit to a single elevation ring of velocity data centered at the radar.
   *
   */

    float getStart(Ray* currentRay);
    /*
   * Returns the environmental wind to be used for a specific ray
   *   for the BB dealiasing algorithm.
   *
   */

    bool BB();
    /*
   * This method is modeled after velocity dealiasing algorithm B,
   *   published by Bargain and Brown (1980).
   *
   */

	bool derivativeDealias();
	/* This method tries to minimize 2nd derivatives in the radial velocity
	 by through velocity unfolding */
	
	bool multiprfDealias();
	/* This method compares rays at different Nyquist velocities for dealiasing */
	
    float findHeight(Ray* currentRay, int gateIndex);
    /*
   * Uses the 4/3 earth radius model to return the height of a specific gate
   *   in km, relative to sea level.
   *
   */

    float **aveVADHeight;
    // aveVADHeight[n][v] : The average height (km from sea level) of the vad
    // ring in sweep n, velocity gate index v.

    float bilinear(float value_high, float value_low,
                   float deltaH_high, float deltaH_low);
    /*
   * Returns a weighted average of two values (value_high & value_low)
   *   relative to their absolute distances from the point of interest
   *   (deltaH_high & deltaH_low)
   *
   */

    void crazyCheck();
    void checkRay();

    //--------------------------Mathematical Methods-------------------------------
    // Moved to Matrix.h

    //-------------------------------------------------------------------------

    float **validBinCount, **last_count_up, **last_count_low;
    bool *vadFound, **hasVelData;
    float *sumwt, *vadRMS;
    int **highVelGate, **lowVelGate;
    bool grad_vad, vad_found, gvad_found;
    int thr, vadthr, gvadthr;
    /*
   *    Varables used by VAD & GVAD Methods
   *
   * validBinCount: two dimensional array that contains the number of none
   *   null doppler velocity measurements in each ring of equadistant gates
   *   within a sweep. (ie validBinCount[sweep1][gate14])
   *
   * last_count_up:
   *
   * last_count_low:
   *
   * vadFound:
   *
   * hasVelData:
   *
   * sumwt:
   *
   * vadRMS:
   *
   * highVelGate:
   *
   * lowVelGate:
   *
   * grad_vad:
   *
   * vad_found:
   *
   * gvad_found:
   *
   * thr:
   *
   * vadthr:
   *
   * gvadthr:
   *
   */


public slots:
    void catchLog(const Message& message);

signals:
    void log(const Message& message);

};


#endif
