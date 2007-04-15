/*
 *  PressureData.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 8/16/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "PressureData.h"
#include <QTextStream>
#include "math.h"

PressureData::PressureData()
{
	latitude = -999;
	longitude = -999;
	altitude = -999;
	pressure = -999;
	windSpeed = -999;
	windDirection = -999;
	stationName = QString();
	time = QDateTime();	
	
}

PressureData::PressureData(const PressureData& other)
{
  latitude = other.latitude;
  longitude = other.longitude;
  altitude = other.altitude;
  pressure = other.pressure;
  windSpeed = other.windSpeed;
  windDirection = other.windDirection;
  stationName = other.stationName;
  time = other.time;

}

PressureData::~PressureData()
{
}

bool PressureData::readObs()
{
	// Virtual function
	return false;
}

QString PressureData::getStationName() const
{
	return stationName;
}

void PressureData::setStationName(const QString& name)
{
	stationName = name;
}

float PressureData::getLat() const
{
	return latitude;
}

void PressureData::setLat(const float& lat)
{
	latitude = lat;
}


float PressureData::getLon() const
{
	return longitude;
}

void PressureData::setLon(const float& lon)
{
	longitude = lon;
}

float PressureData::getAltitude() const
{
	return altitude;
}

void PressureData::setAltitude(const float& alt)
{
	altitude = alt;
}

QDateTime PressureData::getTime() const
{
	return time;
}

void PressureData::setTime(const QDateTime& obTime)
{
	time = QDateTime(obTime);
}

float  PressureData::getPressure() const
{
	return pressure;
}

void  PressureData::setPressure(const float& press)
{
	pressure = press;
}

float  PressureData::getWindSpeed() const
{
	return windSpeed;
}

void  PressureData::setWindSpeed(const float& speed)
{
	windSpeed = speed;
}

float  PressureData::getWindDirection() const 
{
	return windDirection;
}

void  PressureData::setWindDirection(const float& dir)
{
	windDirection = dir;
}

float  PressureData::getUwind() const
{
	float uWind = -windSpeed * sin(windDirection * acos(-1) / 180.);
	return uWind;
}

float  PressureData::getVwind() const
{
	float vWind = -windSpeed * cos(windDirection * acos(-1) / 180.);
	return vWind;
}


bool PressureData::operator ==(const PressureData &other)
{
	if(this->time == other.time)
		return true;
	return false;
}

bool PressureData::operator < (const PressureData &other)
{
	if(this->time < other.time)
		return true;
	return false;
}

bool PressureData::operator > (const PressureData &other)
{
	if(this->time > other.time)
		return true;
	return false;
}
