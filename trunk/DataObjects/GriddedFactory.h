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
		GriddedData* makeEmptyGrid(const char *coordinates);
		GriddedData* makeCappi(RadarData *radarData, 
				       QDomElement cappiConfig,
				       float *vortexLat, float *vortexLon);
		GriddedData* makeAnalytic(RadarData *radarData,
					  QDomElement cappiConfig,
					  Configuration* analyticConfig,
					  float *vortexLat, float *vortexLon);
							   
	private:
		enum coordSystems {
			cartesian,
			cylindrical,
			spherical
		};
		
		coordSystems coordSystem;
};

#endif
