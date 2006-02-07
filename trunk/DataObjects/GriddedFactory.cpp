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

GriddedFactory::GriddedFactory()
{

}

GriddedFactory::~GriddedFactory()
{
}

GriddedData* GriddedFactory::makeEmptyGrid(const char *coordinates)
{

        // Create a new gridded data object with the named coordinate system as the default
        if (coordinates == "cartesian") {
                coordSystem = cartesian;
				/* GriddedData *empty = new GriddedData; */
                return 0;
        } else if (coordinates == "cylindrical") {
                coordSystem = cylindrical;
                /* CylindricalData* cylinData = new CylindricalData; */
        } else if (coordinates == "spherical") {
                coordSystem = spherical;
                /* SphericalData* sphereData = new SphericalData; */
        } else {
                // Not supported
                return 0;
        }
        
        
        
}

GriddedData* GriddedFactory::makeCappi(RadarData *radarData, QDomElement cappiConfig,
		float *vortexLat, float *vortexLon)
{

		coordSystem = cartesian;
		CappiGrid* cappi = new CappiGrid;
		cappi->gridRadarData(radarData,cappiConfig,vortexLat,vortexLon);
		return cappi;
		
}
