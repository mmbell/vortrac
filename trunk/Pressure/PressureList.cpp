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
  //Message::toScreen("Constructor filename = "+fileName);
  workingDir = QString("");
  if(fileName.isEmpty()) {
    fileName = QString("vortrac_defaultPressureListStorage.xml");
    Message::toScreen("Trying to use defaults pressure storage file");
  }

  config = new Configuration(0, newFileName);
  QList<PressureData>::QList<PressureData>();
}

PressureList::PressureList(Configuration* newConfig)
{

  config = newConfig;
  //Message::toScreen("Constructor from Config, root is "+newConfig->getRoot().toElement().tagName());
	
}

PressureList::~PressureList()
{
	
	delete config;
	config = NULL;
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
  //Message::toScreen("Pressure Root "+config->getRoot().toElement().tagName());
  int numPressureObs = 0;
  //Message::toScreen("Opening PressureList count = "+QString().setNum(config->getGroupList()->count()));
  
  for(int i = 0; i < config->getGroupList()->count(); i++) {
    QDomNode node = config->getGroupList()->item(i);
    QDomElement currElement = node.toElement();
    //Message::toScreen(currElement.tagName());
    if(currElement.tagName()==QString("pressure")) {
      vortexName = config->getParam(currElement, "name");
      radarName = config->getParam(currElement, "radar");
      productType = config->getParam(currElement, "product");
    }
    else {
      
      // Has information about a pressureData
      PressureData *newPressure = new PressureData;
      // Station Name
      QString stName = config->getParam(currElement,
					QString("station"));
      if(stName != QString())
	newPressure->setStationName(stName);
      // Latitude
      float newLat = config->getParam(currElement, 
				      QString("latitude")).toFloat();
      if(newLat!=-999)
	newPressure->setLat(newLat);
      // Longitude 
      float newLon = config->getParam(currElement, 
				      QString("longitude")).toFloat();
      if(newLon!=-999)
	newPressure->setLon(newLon);
      // Altitude 
      float newAlt = config->getParam(currElement, 
				      QString("altitude")).toFloat();
      if(newAlt!=-999)
	newPressure->setAltitude(newAlt);
      // DateTime
      QString dateTimeString = config->getParam(currElement, 
						QString("time"));
      QDateTime newTime = QDateTime::fromString(dateTimeString, Qt::ISODate);
      if(newTime.isValid()) {
	newPressure->setTime(newTime);
      }
      
      // Pressure
      float pressureOb = config->getParam(currElement, 
					  QString("pressure")).toFloat();
      if(pressureOb != -999)
	newPressure->setPressure(pressureOb);
	  /* Not using these, so they throw up lots of errors
      // Wind Speed
      float newWindSpeed = config->getParam(currElement, 
					    QString("windspeed")).toFloat();
      if(newWindSpeed!=-999)
	newPressure->setWindSpeed(newWindSpeed);
      // Wind Direction
      float newWindDir = config->getParam(currElement, 
					  QString("winddir")).toFloat();
      if(newWindDir!=-999)
	newPressure->setWindDirection(newWindDir);
      */
      QList<PressureData>::append(*newPressure);
      numPressureObs++;
    }
  }
  if(numPressureObs == config->getGroupList()->count()-1)
    return true;
  else {
    Message::toScreen("Failed to Properly Load All Saved Surface Pressure Observations, Only Loaded "+QString().setNum(numPressureObs));
    return false;
  }
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
  QString stationInfo = "surfaceob_" + newData.getStationName() + "_" + timeString;
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
