/*
 *  MADIS.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "MADIS.h"
#include<QStringList>
#include<QRegularExpression>

MADIS::MADIS()
: PressureData()
{
}

MADIS::MADIS(const QString& ob)
: PressureData()
{
	
  readObs(ob);
	
}

void MADIS::readObs(const QString& ob)
{
    QStringList obList = ob.split(",");
    stationName = obList.at(0);
    QDate obDate = QDate::fromString(obList.at(1), "MM/dd/yyyy");
    QTime obTime = QTime::fromString(obList.at(2), "HH:mm");
    time = QDateTime(obDate, obTime, Qt::UTC);
    QString datestr = obDate.toString(Qt::ISODate);
    QString timestr = obTime.toString(Qt::ISODate);
    //QString timestr = time.toString(Qt::ISODate);
    pressure = obList.at(5).toFloat()/100.0;
    windDirection = obList.at(6).toFloat();
    windSpeed = obList.at(7).toFloat();
    altitude = obList.at(8).toFloat();
    latitude = obList.at(9).toFloat();
    longitude = obList.at(10).toFloat();
    // Prepare the strings for XML
    stationName.replace(QRegularExpression("\\s+"),"");
    stationName.replace(",","");
    stationName.replace(".","");
    stationName.replace("'","");
    stationName.replace("(","");
    stationName.replace(")","");
    stationName.replace("/","_");
    stationName.replace("&","and");
}	
