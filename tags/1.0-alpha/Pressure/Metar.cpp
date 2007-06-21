/*
 *  Metar.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "Metar.h"

Metar::Metar()
: PressureData()
{
}

Metar::Metar(const QString& filename)
: PressureData()
{
	
	obsFile = filename;
	
}

bool Metar::readObs()
{
	
	return false;
	
}
