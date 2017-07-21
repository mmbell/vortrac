/*
 *  GriddedFactory.h
 *  vortrac
 *
 *  Created by Michael Bell on 2/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef GRIDDEDFACTORY_H
#define GRIDDEDFACTORY_H

#include "GriddedData.h"
#include "Config/Configuration.h"

class GriddedFactory
{

public:
    GriddedFactory();
    ~GriddedFactory();
    GriddedData* makeEmptyGrid();
    GriddedData* makeCappi(RadarData *radarData,
                           Configuration* mainConfig,
                           float *vortexLat, float *vortexLon);
    GriddedData* fillPreGriddedData(RadarData *radarData,
				    Configuration* mainConfig);
    GriddedData* makeAnalytic(RadarData *radarData,
                              Configuration* mainConfig,
                              Configuration* analyticConfig,
                              float *vortexLat, float *vortexLon,
                              float *radarLat, float *radarLon);
    // TODO makeRadx (lrose)
    GriddedData* makeRadx(/* TODO */);
    
    void setAbort(volatile bool* newAbort);

private:
    /*	enum coordSystems {
   cartesian,
   cylindrical,
   spherical
  };

  coordSystems coordSystem;
  */

    volatile bool* abort;
};

#endif
