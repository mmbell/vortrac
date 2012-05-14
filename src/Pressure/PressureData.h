/*
 *  PressureData.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 8/16/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef PRESSUREDATA_H
#define PRESSUREDATA_H

#include<QDateTime>
#include<QString>

class PressureData
{
	
public:
	PressureData();
	PressureData(const PressureData& other);
	virtual ~PressureData();
	virtual bool readObs();

	void printString();
	
	QString getStationName() const;
	void setStationName(const QString& name);
	
	float getLat() const;
	void setLat(const float& lat);
	
	float getLon() const;
	void setLon(const float& lon);
	
	float getAltitude() const;
	void setAltitude(const float& alt);
	
	QDateTime getTime() const;
	void setTime(const QDateTime& obTime);
	
	float getPressure() const;
	void setPressure(const float& press);
	
	float getWindSpeed() const;
        void  setWindSpeed(const float& speed);
	float getWindDirection() const;
        void  setWindDirection(const float& dir);
	float getUwind() const;
	float getVwind() const;
	
	bool operator ==(const PressureData &other);
	bool operator < (const PressureData &other);
	bool operator > (const PressureData &other);	

protected:
	
	float latitude;
	float longitude;
	float altitude;
	float pressure;
	float windSpeed;
	float windDirection;
        QDateTime time;
        QString stationName;
};

#endif
