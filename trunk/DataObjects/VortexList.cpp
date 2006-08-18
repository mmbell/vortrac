/*
 * VortexList.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "VortexList.h"
#include <QDomNodeList>

VortexList::VortexList(const QString &newFileName)
{
  fileName = newFileName;
  workingDir = QString("");
  if(fileName.isEmpty())
    fileName = QString("vortrac_defaultVortexDataStorage.xml");

  config = new Configuration(0, newFileName);
  vortexDataConfigs = new QList<Configuration*>;
  configFileNames = new QList<QString>;
  QList<VortexData>::QList<VortexData>();
}

VortexList::VortexList(Configuration* newConfig)
{
  config = newConfig;

  vortexDataConfigs = new QList<Configuration*>;
  configFileNames = new QList<QString>;
}

VortexList::~VortexList()
{
}


bool VortexList::save()
{
  if(config->write(fileName)) {
    int fileSaves = 0;
    for(int i = 0; i < vortexDataConfigs->count(); i++) {
      Configuration *currConfig= vortexDataConfigs->value(i);
      if(currConfig->write(configFileNames->value(i)))
	fileSaves++;
    }
    if(fileSaves == QList<VortexData>::count())
      return true;
  }

  Message::toScreen("VORTEXLIST FAIL:Failed to save to file");
  return false;
}

bool VortexList::open()
{

  int numConfigsOpened = 0;
  // Message::toScreen("num in Config groupList ="+QString().setNum(config->getGroupList()->count()));

  for(int i = 0; i < config->getGroupList()->count(); i++) {

    QDomNode node = config->getGroupList()->item(i);
    QDomElement currElement = node.toElement();
    if(!currElement.tagName().startsWith(QString("vol"))) {
      vortexName = config->getParam(currElement, "name");
      radarName = config->getParam(currElement, "radar");
      productType = config->getParam(currElement, "product");
      numConfigsOpened++;
    }
    else {
      // Has information about a vortexData
      if(openNodeFile(node)) {
	numConfigsOpened++;
      }
    }
  }
  if(numConfigsOpened == config->getGroupList()->count())
    return true;
  Message::toScreen("VORTEXLIST FAIL: Could Not Open All Volume Configuration Files");
  return false;
}

bool VortexList::openNodeFile(const QDomNode &newNode)
{
  if(!newNode.isNull()) {
    QDomElement nodeElement = newNode.toElement();
    QString nodeTimeString = config->getParam(nodeElement, "time");
    QDateTime nodeTime = QDateTime::fromString(nodeTimeString,Qt::ISODate);
    QString nodeFileName = config->getParam(nodeElement, "file");
    if(!QFile::exists(nodeFileName)) {
      Message::toScreen("VORTEXLIST FAIL: File not found for vortexdata: "+nodeFileName);
      return false;
    }
    Configuration *newConfig = new Configuration(0, nodeFileName);
    VortexData newData;

    for(int i = 0; i < newConfig->getGroupList()->count(); i++) {
      QDomNode child = newConfig->getGroupList()->item(i);
      QDomElement childElement = child.toElement();
      QString newName, newRadar, newType;
      if(!childElement.tagName().startsWith(QString("vol"))) {
	newName = newConfig->getParam(childElement, "name");
	newRadar = newConfig->getParam(childElement, "radar");
	newType = newConfig->getParam(childElement, "product");
	// Check to make sure these line up........
      }
      else {
	// Has information about a vortexData;
	QString newTimeString = newConfig->getParam(childElement, "time");
	QDateTime newTime = QDateTime::fromString(newTimeString,Qt::ISODate);
	newData.setTime(newTime);
	float newPressure = newConfig->getParam(childElement,
					     "pressure").toFloat();
	newData.setPressure(newPressure);
	float newPressureUncertainty = newConfig->getParam(childElement,
					     "pressure_uncertainty").toFloat();
	newData.setPressureUncertainty(newPressureUncertainty);
     
	for(int n = 0; n < newData.getNumLevels(); n++) {
	  QString nString = QString().setNum(n);
	  QString level("level");
	  
	  float newLat = newConfig->getParam(childElement, "latitude", level,
					  nString).toFloat();
	  newData.setLat(n, newLat);
	  
	  float newLon = newConfig->getParam(childElement, "longitude", level,
					  nString).toFloat();
	  newData.setLon(n, newLon);

	  float newAlt = newConfig->getParam(childElement, "altitude", level,
					  nString).toFloat();
	  newData.setAltitude(n, newAlt);

	  float newRmw = newConfig->getParam(childElement, "rmw", level,
					  nString).toFloat();
	  newData.setRMW(n, newRmw);
	  
	  float newRmwUncertainty = newConfig->getParam(childElement, 
						     "rmw_uncertainty", 
						     level, 
						     nString).toFloat();
	  newData.setRMWUncertainty(n, newRmwUncertainty);
	  
	  int numCenters = newConfig->getParam(childElement, 
					    "num_converging_centers",level,
					    QString().setNum(n)).toInt();
	  newData.setNumConvergingCenters(n, numCenters);

	  float centerStd = newConfig->getParam(childElement, 
					     "center_std_dev",
					     level, nString).toFloat();
	  newData.setCenterStdDev(n, centerStd);

	  QDomElement coeff = childElement.firstChildElement("coefficient");
	  int num = 0;
	  while(!coeff.isNull()) {
	    Coefficient newCoeff;
	    newCoeff.setLevel(coeff.attribute(level).toInt());
	    newCoeff.setRadius(coeff.attribute("radius").toInt());
	    newCoeff.setParameter(coeff.attribute("parameter"));
	    newData.setCoefficient((int)newCoeff.getLevel(), 
				   (int)newCoeff.getRadius(), 
				   num, newCoeff);
	    coeff = coeff.nextSiblingElement("coefficient");
	    num++;
	  }
	}
	QList<VortexData>::append(newData);
	vortexDataConfigs->append(newConfig);
	configFileNames->append(nodeFileName);
      }
    }
    return true;
  }
  return false;
}

void VortexList::setFileName(const QString &newFileName)
{
  fileName = newFileName;
}

void VortexList::setRadarName(const QString &newRadarName)
{
	radarName = newRadarName;
	QDomElement header = config->getConfig("vortex");
	config->setParam(header, "radar", radarName);

}

void VortexList::setVortexName(const QString &newVortexName)
{
	vortexName = newVortexName;
	QDomElement header = config->getConfig("vortex");
	config->setParam(header, "name", vortexName);

}

void VortexList::setNewWorkingDirectory(const QString &newDirectory)
{
  workingDir = newDirectory;
}

void VortexList::createDomVortexDataEntry(const VortexData &newData)

  /* 
   * This is used to keep the configuration Dom tree consistant
   *   with the vortexData stored in the list. This function commits 
   *   non-null information from the vortexData to the appropriate 
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
  config->addDom(root, QString("volume_"+timeString), QString(""));
  
  QDomElement parent = config->getConfig(QString("volume_"+timeString));
  
  // Create a filename and configuration for the volume information
  
  QString defaultFile = QString("vortrac_defaultVortexListStorage.xml");
  
  //QString nodeFile = vortexName+timeString+QString(".xml");
  //  QFile fileCheck(workingDir+nodeFile);
  //if(QFile::exists(workingDir+nodeFile))
  QString nodeFile = vortexName+"_"+radarName+timeString+QString(".xml");

  Configuration *newConfig = new Configuration(0,defaultFile);

  config->addDom(parent, QString("time"), 
		 newData.getTime().toString(Qt::ISODate)); 
  config->addDom(parent, QString("file"), workingDir+nodeFile);

  vortexDataConfigs->append( newConfig);
  configFileNames->append(workingDir+nodeFile);

  // Add this information to the new catelog entry

  QDomElement volRoot = newConfig->getRoot();

  QDomElement header = newConfig->getConfig("vortex");
  if(!vortexName.isEmpty())
    newConfig->setParam(header, QString("vortexName"), vortexName);
  if(!radarName.isEmpty())
    newConfig->setParam(header, QString("radar"), radarName);
  if(!productType.isEmpty())
    newConfig->setParam(header, QString("product"), productType);
  
  newConfig->addDom(volRoot, QString("volume_"+timeString), QString(""));
  QDomElement volParent =newConfig->getConfig(QString("volume_"+timeString));

  if(!newData.getTime().isNull()) {
    newConfig->addDom(volParent, QString("time"), 
		   newData.getTime().toString(Qt::ISODate)); }
  if(newData.getPressure()!=-999) {
    newConfig->addDom(volParent, QString("pressure"), 
		   QString().setNum(newData.getPressure())); }
  if(newData.getPressureUncertainty()!=-999) {
    newConfig->addDom(volParent, QString("pressure_uncertainty"),
		   QString().setNum(newData.getPressureUncertainty())); }


  for(int i = 0; i < newData.getNumLevels(); i++) {
    QString ii = QString().setNum(i);
    QString level("level");
    if(newData.getLat(i)!=-999)  {
      newConfig->addDom(volParent, QString("latitude"), 
		     QString().setNum(newData.getLat(i)), level, ii); }
    if(newData.getLon(i)!=-999)  {
      newConfig->addDom(volParent, QString("longitude"),
		     QString().setNum(newData.getLon(i)), level, ii); }
    if(newData.getAltitude(i)!=-999) {
      newConfig->addDom(volParent, QString("altitude"),
		     QString().setNum(newData.getAltitude(i)), level, ii); }
    if(newData.getRMW(i)!=-999)  {
      newConfig->addDom(volParent, QString("rmw"),
		     QString().setNum(newData.getRMW(i)), level, ii); }
    if(newData.getRMWUncertainty(i)!=-999) {
      newConfig->addDom(volParent, QString("rmw_uncertainty"), 
	     QString().setNum( newData.getRMWUncertainty(i)), level, ii); }
    if(newData.getNumConvergingCenters(i)!=-999) {
      newConfig->addDom(volParent, QString("num_converging_centers"),
		     QString().setNum(newData.getNumConvergingCenters(i)),
		     level, ii); }
    if(newData.getCenterStdDev(i)!=-999) {
      newConfig->addDom(volParent, QString("center_std_dev"),
		     QString().setNum(newData.getCenterStdDev(i)), level, ii);}

    for(int r = 0; r < newData.getNumRadii(); r++) {
      for(int w = 0; w < newData.getNumWaveNum(); w++) {
	QString ff("coefficient");
	QString rr = QString().setNum(r);
	QString paramString("parameter");
	Coefficient newCoeff = newData.getCoefficient(i,r,w); 
	if(!newCoeff.isNull()) {
	  newConfig->addDom(volParent, ff, 
			    QString().setNum(newCoeff.getValue()), 
			    paramString, newCoeff.getParameter());
	  QDomElement coeffDom = newConfig->getElementWithAttrib(volParent, 
				ff, paramString, newCoeff.getParameter());
	  coeffDom.setAttribute(level, ii);
	  coeffDom.setAttribute(QString("radius"), rr);
	}
      }
    }
  }
}

void VortexList::append(const VortexData &value)
{
  createDomVortexDataEntry(value);
  QList<VortexData>::append(value);
}

void VortexList::timeSort()
{
  for(int i = 0; i < this->count(); i++) {
    for(int j = 0; j < this->count()-1; j++) {
      if(this->value(j)>this->value(j+1)) {
	this->swap(j+1,j);
	vortexDataConfigs->swap(j+1,j);
	configFileNames->swap(j+1,j);
      }
    }
  }
  
}
