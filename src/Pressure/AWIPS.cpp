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
#include<QRegularExpression>

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
  uint unixTime = (int)ob.left(15).toFloat();
  time.setTimeSpec(Qt::UTC);
  time.setSecsSinceEpoch(unixTime);
  latitude = ob.mid(17,5).toFloat();
  longitude = ob.mid(24,7).toFloat();
  pressure = ob.mid(33,6).toFloat();
  // Prepare the strings for XML
  stationName = ob.mid(41).replace(QRegularExpression("\\s+"),"_");
  stationName.replace(",","");
  stationName.replace(".","");
  stationName.replace("'","");
  stationName.replace("(","");
  stationName.replace(")","");
  stationName.replace("/","_");
  stationName.replace("&","and");
  windSpeed = -999;
  windDirection = -999;
  altitude = 10;

}	
