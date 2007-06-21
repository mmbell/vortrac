/*
 * PressureList.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "PressureList.h"
#include <QDomNodeList>

PressureList::PressureList(const QString &newFileName)
{
  fileName = newFileName;
  workingDir = QString("");
  if(fileName.isEmpty())
    fileName = QString("vortrac_defaultPressureListStorage.xml");

  config = new Configuration(0, newFileName);
  QList<PressureData>::QList<PressureData>();
}

PressureList::PressureList(Configuration* newConfig)
{

	config = newConfig;
	
}

PressureList::~PressureList()
{
	
	delete config;
	
}


bool PressureList::save()
{
  if(config->write(fileName)) {
      return true;
  }

  Message::toScreen("PRESSURELIST FAIL:Failed to save to file");
  return false;
}

bool PressureList::open()
{
  // Do we still need this if all the elements are in one file?
  return true;
}

bool PressureList::openNodeFile(const QDomNode &newNode)
{
  return false;
}

void PressureList::setFileName(const QString &newFileName)
{
  fileName = newFileName;
}

void PressureList::setRadarName(const QString &newRadarName)
{
	radarName = newRadarName;
	QDomElement header = config->getConfig("pressure");
	config->setParam(header, "radar", radarName);
}

void PressureList::setVortexName(const QString &newVortexName)
{
	vortexName = newVortexName;
	QDomElement header = config->getConfig("pressure");
	config->setParam(header, "name", vortexName);

}

void PressureList::setNewWorkingDirectory(const QString &newDirectory)
{
  workingDir = newDirectory;
}

void PressureList::createDomPressureDataEntry(const PressureData &newData)

  /* 
   * This is used to keep the configuration Dom tree consistant
   *   with the pressureData stored in the list. This function commits 
   *   non-null information from the pressureData to the appropriate 
   *   element in its configuration node counterpart.
   *
   */

{

  // Get time for identification

  QString timeString = newData.getTime().toString(Qt::ISODate);
  timeString.replace(QString("-"), QString("_"));
  timeString.replace(QString(":"), QString("_"));
 
  // Add element to main xml identifing specific file

  QDomElement root = config->getRoot();
  QString stationInfo = "surfaceob_" + newData.getStationName() + "_" + newData.getTime().toString(Qt::ISODate);
  config->addDom(root, stationInfo, QString(""));
  
  QDomElement parent = config->getConfig(stationInfo);
  
  // Add this information to the new catelog entry
  if(!newData.getStationName().isNull()) {
	  config->addDom(parent, QString("station"), 
						newData.getStationName()); }
  if(!newData.getTime().isNull()) {
    config->addDom(parent, QString("time"), 
		   newData.getTime().toString(Qt::ISODate)); }
  if(newData.getPressure()!=-999) {
    config->addDom(parent, QString("pressure"), 
		   QString().setNum(newData.getPressure())); }
  if(newData.getLat()!=-999) {
    config->addDom(parent, QString("latitude"), 
					  QString().setNum(newData.getLat())); }
  if(newData.getLon()!=-999) {
	  config->addDom(parent, QString("longitude"), 
					  QString().setNum(newData.getLon())); }
  if(newData.getPressure()!=-999) {
    config->addDom(parent, QString("altitude"), 
					  QString().setNum(newData.getAltitude())); }
  if(newData.getWindSpeed()!=-999) {
    config->addDom(parent, QString("windspeed"), 
					  QString().setNum(newData.getWindSpeed())); }
  if(newData.getWindDirection()!=-999) {
    config->addDom(parent, QString("winddir"), 
					  QString().setNum(newData.getWindDirection())); }

  
}

void PressureList::append(const PressureData &value)
{
  createDomPressureDataEntry(value);
  QList<PressureData>::append(value);
}

void PressureList::timeSort()
{
	/* Need to redo this function
  for(int i = 0; i < this->count(); i++) {
    for(int j = 0; j < this->count()-1; j++) {
      if(this->value(j)>this->value(j+1)) {
	this->swap(j+1,j);
	pressureDataConfigs->swap(j+1,j);
	configFileNames->swap(j+1,j);
      }
    }
  }
  */
}
