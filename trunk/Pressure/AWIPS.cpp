/*
 *  AWIPS.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "AWIPS.h"
#include<QStringList>

AWIPS::AWIPS()
: PressureData()
{
}

AWIPS::AWIPS(const QString& ob)
: PressureData()
{
	
	readObs(ob);
	
}

void AWIPS::readObs(const QString& ob)
{
	
	latitude = ob.mid(12,7).toFloat();
	longitude = ob.mid(19,9).toFloat();
	pressure = ob.mid(28,8).toFloat();
	stationName = ob.mid(36);
	
	windSpeed = -999;
	windDirection = -999;
	altitude = 10;

}	
