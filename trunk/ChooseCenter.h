/*
 * ChooseCenter.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/30/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef CHOOSECENTER_H
#define CHOOSECENTER_H

#include "Configuration.h"
#include "DataObjects/SimplexList.h"
#include "DataObjects/VortexData.h"
#include "Math/Matrix.h"
#include <QDateTime>

class ChooseCenter 
{

 public:
     ChooseCenter(Configuration* newConfig = new Configuration(), 
				  const SimplexList &newList = SimplexList(),
				  VortexData* vortexPtr = VortexData());
    ~ChooseCenter();
  
    void setConfig(Configuration* newConfig);
    void setSimplexList(const SimplexList &newList);
    bool findCenter();
    void initialize();
    bool chooseMeanCenters();
    bool constructPolynomial();
    bool fixCenters();
    bool fixCentersNoFit();
    // void checkHeights();

  
 private:
    Configuration* config;       // Should this be a constant parameter
    SimplexList simplexResults;
	VortexData* vortexData;
    float velNull;
  
    float windWeight, stdWeight, ptsWeight;
    float positionWeight, rmwWeight, velWeight;
    float fCriteria[30];
    QDateTime startTime, endTime;
    /* 
     * windWeight is a user set parameter which dictates the relative 
     *   importance of windSpeed to the initial rmw selection
     *
     * stdWeight is a user set parameter which dictates the relative
     *   importance of the center's standard deviation to the initial rmw
     *   selection
     *
     * ptsWeight is a user set parameter which dictates the relative 
     *   importance of the number of converging centers to the initial rmw
     *   selection
     *
     * positionWeight is a user set parameter which dictates the relative 
     *   importance of the x and y scores in selecting a center that best
     *   matches the line of best fit
     *
     * rmwWeight is a user set parameter which dictates the relative 
     *   importance of the rmw score in selecting a center that best matches 
     *   the line of best fit
     *
     * velWeight is a user set parameter which dictates the relative 
     *   importance of the velocity score in selecting a center that best
     *   matches the line of best fit
     *
     * fCriteria is an array used internally to preform the fTest on data
     *   that is used obtaining the curve of best fit. These values were 
     *   from a text
     *
     * startTime and endTime contain the user entered parameters controlling
     *   which volumes are used in the choose center process
     *
     */

    float*** score;
    int **bestRadius;
    /*
     * bestRadius contains the index of the best radius for each level of 
     *   each volume used. Here the best radius is decided from examining the
     *   means of all converging centers used in the simplex run.
     *   bestRadius[# of volumes used][# of levels in each volume] 
     *
     */

    float *centerDev, *radiusDev;
    float **bestFitVariance;
    int **bestFitDegree;
    float ***bestFitCoeff;
    /*
     * centerDev holds the standard deviation of the center of the storm
     *   for an averaged center position that is created from all of the 
     *   converging centers used in simplex
     *   centerDev[# of volumes used]
     *
     * radiusDev holds the standard deviation of the radius of maximum wind
     *   for an averaged radius of maximum wind generated from all of the 
     *   converging centers used in simplex
     *   radiusDev[# of volumes used]
     *
     * bestFitVariance holds the floating point variance of the fit for
     *   each characteristic that was fitted for each level
     *   bestFitVariance[4][# of levels in all volumes]
     *
     * bestFitDegree holds an integer corresponding to the maximum order
     *   of the best fitting polynomial for each characteristic that was
     *   fitted for each level
     *   bestFitDegree[4][# of levels in all volumes]
     *
     * bestFitCoeff holds the floating point number that represents a 
     *   coefficient that corresponds to the line of fit for each 
     *   characteristic that was fitted at each level for each order
     *   of polynomial in the fit
     *   bestFitCoeff[4][# of levels in all volumes][order of polynomial]
     *
     * characteristics fitted at each level within each volume are:
     *  1: x coordinate in cappi grid space
     *  2: y coordinate in cappi grid space
     *  3: radius of maximum wind found
     *  4: maximum wind speed in km/s
     *
     */

    int **newBestRadius, **newBestCenter;
    /*
     * newBestRadius holds the interger index of the radius that provides the
     *   highest score within the given criteria for each volume and level
     *   newBestRadius[number of volumes used][# levels in each volume] 
     * newBestCenter holds the interger index of the center that provides
     *   the highest score within the given criteria for each volume and level
     *   newCenterRadius[number of volumes used][# levels in each volume]
     *
     */

    /*
     * In finding the individual centers of interest (fixCenters) many 
     *   quantities are generated and printed to screen but not kept in 
     *   storage in the original perl code. I implemented this part of the
     *   code just as perl did with the exception of writing any of the data 
     *   to screen, so several of these quantities are forgetten right after 
     *   they are generated. If you want any of these for later use we will 
     *   have set up member arrays. 
     *   Specific examples include: newVariance, centerDeviation,
     *                              radiusDeviation ...
     *   all of these are toward the end of the ChooseCenter.cpp (~700)
     *
     */
    
    int minVolumes;



};

#endif
