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
#include <QDir>
#include <math.h>

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
  configFileNames->clear();
  while(vortexDataConfigs->count() > 0)
    delete vortexDataConfigs->takeLast();
  delete configFileNames;
  delete vortexDataConfigs;
  delete config;
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
    Message::toScreen("VORTEXLIST FAIL:Failed to save all the files");
  }

  Message::toScreen("VORTEXLIST FAIL:Failed to save to file");
  return false;
}

bool VortexList::saveNodeFile(int index, const QString& newName)
{
  if(index >= this->count())
    return false;
  if(QDir(newName).isAbsolute()) {
    if(vortexDataConfigs->at(index)->write(newName))
      return true;
    return false;
  }
  else {
    if(vortexDataConfigs->at(index)->write(QDir::current().filePath(newName)))
      return true;
    return false;
  }
}

bool VortexList::open()
{

  int numConfigsOpened = 0;
  //Message::toScreen("num in Config groupList ="+QString().setNum(config->getGroupList()->count()));

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
    QStringList timeParts =  nodeTimeString.remove("volume_").split("T");
    if(timeParts.count()!=2){
      Message::toScreen(QString("OpenNodeFile: Cannot Read Volume Times in: "+fileName));
      return false;
    }
    QDate nodeDate = QDate::fromString(timeParts[0],Qt::ISODate);
    QTime nodeTime = QTime::fromString(timeParts[1],Qt::ISODate);
    QDateTime nodeDateTime = QDateTime(nodeDate, nodeTime, Qt::UTC);
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
	if((newName!=vortexName)||(newRadar!=radarName)) {
	  Message::toScreen(QString("Difficulty Loading VortexList File: "+nodeFileName+", this file may have been altered or incomplete"));
	}
      }
      else {
	// Has information about a vortexData;
	QString newTimeString = newConfig->getParam(childElement, "time");
	QStringList timeParts =  newTimeString.remove("volume_").split("T");
	if(timeParts.count()!=2){
	  Message::toScreen("Failing to read the volume times correctly");
	  return false;
	}
	//Message::toScreen("Loading Simplex Run "+nodeTimeString);
	QDate nodeDate = QDate::fromString(timeParts[0],Qt::ISODate);
	QTime nodeTime = QTime::fromString(timeParts[1],Qt::ISODate);
	QDateTime nodeDateTime = QDateTime(nodeDate, nodeTime, Qt::UTC);
	newData.setTime(nodeDateTime);
	float newPressure = newConfig->getParam(childElement,
					     "pressure").toFloat();
	newData.setPressure(newPressure);
	float newPressureUncertainty = newConfig->getParam(childElement,
					     "pressure_uncertainty").toFloat();
	newData.setPressureUncertainty(newPressureUncertainty);
	float newPressureDeficit = newConfig->getParam(childElement,
						 "pressure_deficit").toFloat();
	newData.setPressureDeficit(newPressureDeficit);
	float newDeficitUncertainty = newConfig->getParam(childElement,
							  "deficit_uncertainty").toFloat();
	newData.setDeficitUncertainty(newDeficitUncertainty);
	float newAveRMW = newConfig->getParam(childElement,
					      "ave_rmw").toFloat();
	newData.setAveRMW(newAveRMW);
	float newAveRMWUncertainty = newConfig->getParam(childElement,
							 "ave_rmw_uncertainty").toFloat();
	newData.setAveRMWUncertainty(newAveRMWUncertainty);

	int highestLevel = 0;

	for(int n = 0; n < newData.getMaxLevels(); n++) {
	  QString nString = QString().setNum(n);
	  QString level("level");
	  
	  QString newLat = newConfig->getParam(childElement, "latitude", level,
					  nString);
	  if(newLat!=QString()) {
	    newData.setLat(n, newLat.toFloat()); 
	    highestLevel = n;}

	  
	  QString newLon = newConfig->getParam(childElement,"longitude", level,
					  nString);
	  if(newLon!=QString()) {
	    newData.setLon(n, newLon.toFloat());
	    highestLevel = n;}

	  QString newAlt = newConfig->getParam(childElement, "altitude", level,
					  nString);
	  if(newAlt!=QString()) {
	    newData.setHeight(n, newAlt.toFloat());
	    highestLevel = n;}

	  QString newRmw = newConfig->getParam(childElement, "rmw", level,
					  nString);
	  if(newRmw!=QString()) {
	    newData.setRMW(n, newRmw.toFloat());
	    highestLevel = n;}
	  
	  QString newRmwUncertainty = newConfig->getParam(childElement, 
							  "rmw_uncertainty", 
							  level, 
							  nString);
	  if(newRmwUncertainty!=QString()) {
	    newData.setRMWUncertainty(n, newRmwUncertainty.toFloat());
	    highestLevel = n;}
	  
	  QString numCenters = newConfig->getParam(childElement, 
					    "num_converging_centers",level,
					    QString().setNum(n));
	  if(numCenters!=QString()) {
	    newData.setNumConvergingCenters(n, numCenters.toInt());
	    highestLevel = n;}

	  QString centerStd = newConfig->getParam(childElement, 
					     "center_std_dev",
					     level, nString);
	  if(centerStd!=QString()) {
	    newData.setCenterStdDev(n, centerStd.toFloat());
	    highestLevel = n;}

	}
	newData.setNumLevels(highestLevel+1);

	QDomElement coeff = childElement.firstChildElement("coefficient");
	int num[newData.getMaxLevels()][newData.getMaxRadii()];
	for(int level = 0; level < newData.getMaxLevels(); level++) {
	  for(int rad = 0; rad < newData.getMaxRadii(); rad++) {
	    num[level][rad] = 0;
	  }
	}
	int currNumWaveNum = 0;
	int currCoefficient = 0;
	int currNumRadii = 0;
	while((!coeff.isNull())&&(currCoefficient <= newData.getMaxWaveNum()*2+3)) {
	  Coefficient newCoeff;
	  int thisLevel = coeff.attribute(QString("level")).toInt();
	  int thisRadius = coeff.attribute(QString("radius")).toInt();
	  newCoeff.setLevel(thisLevel);
	  newCoeff.setRadius(thisRadius);
	  num[thisLevel][thisRadius]++;
	  if(num[thisLevel][thisRadius] > currCoefficient)
	    currCoefficient = num[thisLevel][thisRadius];
	  if(thisRadius > currNumRadii)
	    currNumRadii = thisRadius;
	  newCoeff.setParameter(coeff.attribute("parameter"));
	  if(coeff.text()!=QString()) {
	    newCoeff.setValue(coeff.text().toFloat());
	    newData.setCoefficient((int)newCoeff.getLevel(), 
				   (int)newCoeff.getRadius(), 
				   num[thisLevel][thisRadius], newCoeff);
	  }
	  coeff = coeff.nextSiblingElement("coefficient");
	}
	currNumWaveNum = int((currCoefficient-3)/2.0+1);
	newData.setNumWaveNum(currNumWaveNum);
	newData.setNumWaveNum(currNumRadii+1);
	
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

void VortexList::setProductType(const QString &newProductType)
{
  productType = newProductType;
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

  vortexDataConfigs->append(newConfig);
  configFileNames->append(workingDir+nodeFile);

  // Add this information to the new catelog entry

  QDomElement volRoot = newConfig->getRoot();

  QDomElement header = newConfig->getConfig("vortex");
  if(!vortexName.isEmpty())
    newConfig->setParam(header, QString("name"), vortexName);
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
  if(newData.getPressureDeficit()!=-999) {
    newConfig->addDom(volParent, QString("pressure_deficit"), 
		      QString().setNum(newData.getPressureDeficit())); }
  if(newData.getDeficitUncertainty()!=-999) {
    newConfig->addDom(volParent, QString("deficit_uncertainty"),
		      QString().setNum(newData.getDeficitUncertainty())); }
  if(newData.getAveRMW()!=-999) {
    newConfig->addDom(volParent, QString("ave_rmw"),
		      QString().setNum(newData.getAveRMW())); }
  if(newData.getAveRMWUncertainty()!=-999) { 
    newConfig->addDom(volParent, QString("ave_rmw_uncertainty"),
		      QString().setNum(newData.getAveRMWUncertainty())); }

  for(int i = 0; i < newData.getNumLevels(); i++) {
    QString ii = QString().setNum(i);
    QString level("level");
    if(newData.getLat(i)!=-999)  {
      newConfig->addDom(volParent, QString("latitude"), 
		     QString().setNum(newData.getLat(i)), level, ii); }
    if(newData.getLon(i)!=-999)  {
      newConfig->addDom(volParent, QString("longitude"),
		     QString().setNum(newData.getLon(i)), level, ii); }
    if(newData.getHeight(i)!=-999) {
      newConfig->addDom(volParent, QString("altitude"),
		     QString().setNum(newData.getHeight(i)), level, ii); }
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
      for(int w = 0; w < newData.getNumWaveNum()*2+3; w++) {
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

void VortexList::removeAt(int i)
{
  int initialCount = vortexDataConfigs->count();
  configFileNames->removeAt(i);
  delete vortexDataConfigs->takeAt(i);
  if(vortexDataConfigs->count() >= initialCount)
    Message::toScreen(QString("RemoveAt didn't reduce the number of data points in the list"));
  QString timeString = this->at(i).getTime().toString(Qt::ISODate);
  timeString.replace(QString("-"), QString("_"));
  timeString.replace(QString(":"), QString("_"));
  timeString.chop(2);
  timeString = "volume_"+timeString;
  timeString = config->findConfigNameStartsWith(timeString);
  // Remove element and children from main xml identifing specific file

  QDomElement parent = config->getConfig(timeString);
  if(parent.tagName()!=timeString)
    Message::toScreen("The tag names are different parent: "+parent.tagName()+" what we got "+timeString);
  config->removeDom(parent, QString("time")); 
  config->removeDom(parent, QString("file"));
  QDomElement root = config->getRoot();
  config->removeDom(root, timeString);

  QList<VortexData>::removeAt(i);
}

void VortexList::timeSort()
{
  for(int i = 0; i < this->count(); i++) {
    for(int j = i+1; j < this->count(); j++) {
      if(this->at(i).getTime()>this->at(j).getTime()) {
	this->swap(j,i);
	vortexDataConfigs->swap(j,i);
	configFileNames->swap(j,i);
      }
    }
  }
  
}

void VortexList::setIndividualProductType(int dataIndex,const QString& newType)
{
  QDomElement vortex = vortexDataConfigs->at(dataIndex)->getConfig("vortex");
  vortexDataConfigs->at(dataIndex)->setParam(vortex, 
					     QString("product"), newType);
}
