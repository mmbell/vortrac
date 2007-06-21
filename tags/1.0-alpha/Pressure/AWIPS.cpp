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
	
	QStringList obList = ob.split(",");
	stationName = obList.at(1);
	QDate obDate = QDate(obList.at(2).left(4).toInt(),
						 obList.at(2).mid(4,2).toInt(),
						 obList.at(2).right(2).toInt());
	
	QTime obTime = QTime(obList.at(3).left(2).toInt(),
						 obList.at(3).mid(2,2).toInt(),
						 obList.at(3).right(2).toInt());
	time.setDate(obDate);
	time.setTime(obTime);
	time.setTimeSpec(Qt::UTC);
	latitude = obList.at(4).toFloat();
	longitude = obList.at(5).toFloat() - 360.;
	windSpeed = obList.at(6).toFloat();
	windDirection = obList.at(7).toFloat();
	pressure = obList.at(8).toFloat();
	altitude = obList.at(9).toFloat();

}	