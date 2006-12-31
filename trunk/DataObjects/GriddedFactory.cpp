/*
 *  GriddedFactory.cpp
 *  vortrac
 *
 *  Created by Michael Bell on 2/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "GriddedFactory.h"
#include "CappiGrid.h"
#include "AnalyticGrid.h"

GriddedFactory::GriddedFactory()
{
  abort = NULL;
}

GriddedFactory::~GriddedFactory()
{
}

GriddedData* GriddedFactory::makeEmptyGrid(const char *coordinates)
{

        // Create a new gridded data object with the named coordinate system as the default
        if (coordinates == "cartesian") {
	  //coordSystem = cartesian;
				/* GriddedData *empty = new GriddedData; */
                return 0;
        } else if (coordinates == "cylindrical") {
	  // coordSystem = cylindrical;
                /* CylindricalData* cylinData = new CylindricalData; */
        } else if (coordinates == "spherical") {
	  //    coordSystem = spherical;
                /* SphericalData* sphereData = new SphericalData; */
        } else {
                // Not supported
                return 0;
        }
        
        
}

GriddedData* GriddedFactory::makeCappi(RadarData *radarData, 
				       Configuration* mainConfig,
				       float *vortexLat, float *vortexLon)
{

  //coordSystem = cartesian;
		CappiGrid* cappi = new CappiGrid;
		cappi->setExit(abort);
		cappi->gridRadarData(radarData,mainConfig->getConfig("cappi"),
				     vortexLat,vortexLon);
		return cappi;
		
}

GriddedData* GriddedFactory::makeAnalytic(RadarData *radarData,
					  Configuration* mainConfig, 
					  Configuration* analyticConfig,
					  float *vortexLat, float *vortexLon,
					  float *radarLat, float *radarLon)
{
                
                if(radarData->getNumRays() <= 0) {
		  //coordSystem = cartesian;
		     AnalyticGrid *data = new AnalyticGrid();
		     data->setExit(abort);
		     data->gridAnalyticData(mainConfig, analyticConfig, 
					    vortexLat, vortexLon,radarLat,
					    radarLon);
		     return data;
		}
		else {
		    return makeCappi(radarData, mainConfig, 
				     vortexLat, vortexLon);
		}
		
		Message::toScreen("Error in make Analytic GriddedFactory");
		return new CappiGrid();
	      
}

void GriddedFactory::setAbort(volatile bool* newAbort)
{
  abort = newAbort;
}
