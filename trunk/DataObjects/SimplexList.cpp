/*
 * SimplexList.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "SimplexList.h"
#include <QDomNodeList>
#include <QDir>

SimplexList::SimplexList(const QString &newFileName)
{
  fileName = newFileName;
  workingDir = QString("");
  if(fileName.isEmpty())
    fileName = QString("vortrac_defaultSimplexListStorage.xml");

  config = new Configuration(0, newFileName);
  simplexDataConfigs = new QList<Configuration*>;
  configFileNames = new QList<QString>;
  QList<SimplexData>::QList<SimplexData>();
}

SimplexList::SimplexList(Configuration* newConfig)
{
  config = newConfig;

  simplexDataConfigs = new QList<Configuration*>;
  configFileNames = new QList<QString>;
}

SimplexList::~SimplexList()
{
  delete config;
  for(int i = simplexDataConfigs->count()-1; i >=0; i--)
    delete simplexDataConfigs->value(i);
  delete simplexDataConfigs;
  configFileNames->clear();
  delete configFileNames;
}


bool SimplexList::save()
{
  if(config->write(fileName)) {
    int fileSaves = 0;
    for(int i = 0; i < simplexDataConfigs->count(); i++) {
      Configuration *currConfig= simplexDataConfigs->value(i);
      if(currConfig->write(configFileNames->value(i)))
	fileSaves++;
      //Message::toScreen(QString().setNum(fileSaves));
    }
    if(fileSaves == QList<SimplexData>::count()) {
      Message::toScreen(QString().setNum(fileSaves));
      return true;
    }
    Message::toScreen("SIMPLEXLIST FAIL:Failed to save all subFiles");
  }

  Message::toScreen("SIMPLEXLIST FAIL:Failed to save to file");
  return false;
}

bool SimplexList::saveNodeFile(int index, const QString& newName)
{
  if(index >= this->count())
    return false;
  if(QDir(newName).isAbsolute()) {
    if(simplexDataConfigs->at(index)->write(newName))
      return true;
    return false;
  }
  else {
    if(simplexDataConfigs->at(index)->write(QDir::current().filePath(newName)))
      return true;
    return false;
  }
}


bool SimplexList::open()
{

  int numConfigsOpened = 0;
  //Message::toScreen("!num in Config groupList ="+QString().setNum(config->getGroupList()->count()));

  for(int i = 0; i < config->getGroupList()->count(); i++) {

    QDomNode node = config->getGroupList()->item(i);
    QDomElement currElement = node.toElement();
    if(!currElement.tagName().startsWith(QString("vol"))) {
      // Simplex Node General Parameters
      vortexName = config->getParam(currElement, "name");
      radarName = config->getParam(currElement, "radar");
      productType = config->getParam(currElement, "product");
      numConfigsOpened++;
      //Message::toScreen("found the header config for index");
    }
    else {
      // Has information about a simplexData
      //Message::toScreen("found data node");
      if(openNodeFile(node)) {
	numConfigsOpened++;
      }
    }
  }
  if(numConfigsOpened == config->getGroupList()->count())
    return true;
  Message::toScreen("SIMPLEXLIST FAIL: Could Not Open All Volume Configuration Files");
  return false;
}

bool SimplexList::openNodeFile(const QDomNode &newNode)			    
{
  
  if(!newNode.isNull()) {
    QDomElement nodeElement = newNode.toElement();
    QString nodeTimeString = config->getParam(nodeElement, "time");
    QStringList timeParts =  nodeTimeString.remove("volume_").split("T");
    if(timeParts.count()!=2){
      Message::toScreen("Failing to read the volume times correctly");
      return false;
    }
    //Message::toScreen("Loading Simplex Run "+nodeTimeString);
    QDate nodeDate = QDate::fromString(timeParts[0],Qt::ISODate);
    QTime nodeTime = QTime::fromString(timeParts[1],Qt::ISODate);
    QDateTime nodeDateTime = QDateTime(nodeDate, nodeTime, Qt::UTC);
    QString nodeFileName = config->getParam(nodeElement, "file");
    if(!QFile::exists(nodeFileName)) {
      Message::toScreen("SIMPLEXLIST FAIL: File not found for simplexdata: "+nodeFileName);
      return false;
    }
    Configuration *newConfig = new Configuration(0, nodeFileName);
    SimplexData newData;
    int centIndex[15][30];
    for(int i = 0; i < 15; i++) {
      for(int j = 0; j < 30; j++) {
	centIndex[i][j] = 0;
      }
    }
    
    for(int i = 0; i < newConfig->getGroupList()->count(); i++) {
      QDomNode child = newConfig->getGroupList()->item(i);
      QDomElement childElement = child.toElement();
      //Message::toScreen("child Element = "+childElement.tagName());
      QString newName, newRadar, newType, newTime;
      if(childElement.tagName().startsWith(QString("simplex"))) {
	newName = newConfig->getParam(childElement, "name");
	newRadar = newConfig->getParam(childElement, "radar");
	newType = newConfig->getParam(childElement, "product");
	newTime = newConfig->getParam(childElement, "time");
	QStringList timeParts =  newTime.remove("volume_").split("T");
	if(timeParts.count()!=2){
	  Message::toScreen("SimplexList: OpenNodeFile: Error - Failing to read the volume times correctly");
	  return false;
	}
	//Message::toScreen("Loading Simplex Run "+nodeTimeString);
	QDate nodeDate = QDate::fromString(timeParts[0],Qt::ISODate);
	QTime nodeTime = QTime::fromString(timeParts[1],Qt::ISODate);
	QDateTime nodeDateTime = QDateTime(nodeDate, nodeTime, Qt::UTC);
	newData.setTime(nodeDateTime);
	newData.setNumPointsUsed(newConfig->getParam(childElement, 
						     "numpoints").toInt());
	// Check to make sure these line up........
      }
      else {
	// This node belongs to a specific search or level average
	int level = childElement.attribute("level").toInt();
	if(level >= newData.getMaxLevels()) {
	  //  emit log(Message(QString("Maximum Number of Levels Available In Memory Overrun When Trying to Load Simplex From File"), 0, this->objectName()));
	  Message::toScreen(QString("Maximum Number of Levels Available In Memory Overrun When Trying to Load Simplex From File"));
	  continue;
	}
     
	int radius = childElement.attribute("radius").toInt();
	if(radius >= newData.getMaxRadii()) {
	  // emit log(Message(QString("Maximum Number of Radii Available In Memory Overrun When Trying to Load Simplex From File"), 0, this->objectName()));
	  Message::toScreen(QString("Maximum Number of Radii Available In Memory Overrun When Trying to Load Simplex From File"));
	  continue;
	}
	
	if(childElement.tagName().startsWith(QString("sum"))) {

	  // Sum Node average parameters

	  newData.setX(level, radius, 
		       config->getParam(childElement, 
					QString("meanx")).toFloat());
	  newData.setY(level, radius, 
		       config->getParam(childElement, "meany").toFloat());
	  newData.setHeight(level,
			   config->getParam(childElement, "height").toFloat());
	  newData.setRadius(radius,
				config->getParam(childElement, "radius").toFloat());
	  newData.setCenterStdDev(level, radius, 
		  config->getParam(childElement, "center_stddev").toFloat());
	  newData.setMaxVT(level, radius, 
			   config->getParam(childElement, "meanvt").toFloat());
	  newData.setVTUncertainty(level, radius, 
		   config->getParam(childElement, "meanvt_stddev").toFloat());
	  newData.setNumConvergingCenters(level, radius, 
		   config->getParam(childElement, "numcenters").toInt());
	}
	else {
	  
	  // Has information about the specific centers used in search

	  QDomElement centerParam = childElement.firstChildElement();
	  while(!centerParam.isNull()) {
	    if(centerParam.tagName()!=QString("x")) {
	      // Only record each center once 
	      // we are taking queues from the number of x elements
	      centerParam = centerParam.nextSiblingElement();
	      continue;
	    }
	    Center newCenter;
	    
	    QString centerString = centerParam.attribute("center");
	    int centerIndex = centerString.toInt();
	    if(centerIndex >= newData.getMaxCenters()) {
	      // emit log(Message(QString("Maximum Number of Centers Available In Memory Overrun When Trying to Load Simplex From File"), 0, this->objectName()));
	      Message::toScreen(QString("Maximum Number of Centers Available In Memory Overrun When Trying to Load Simplex From File"));
	      continue;
	    }
	    newCenter.setLevel(level);
	    newCenter.setRadius(radius);
	    QDomElement xElem = newConfig->getElementWithAttrib(childElement, 
					 "x","center",centerString);
	    newCenter.setX(xElem.text().toFloat());
	    float x0 = xElem.attribute("x0").toFloat();
	    newData.setInitialX(level, radius, centerIndex, x0);
	    
	    QDomElement yElem = newConfig->getElementWithAttrib(childElement, 
					 "y","center",centerString);
	    newCenter.setY(yElem.text().toFloat());
	    float y0 = yElem.attribute("y0").toFloat();
	    newData.setInitialY(level, radius, centerIndex, y0);

	    QDomElement vElem = newConfig->getElementWithAttrib(childElement, 
				     "maxvt","center",centerString);
	    newCenter.setMaxVT(vElem.text().toFloat());
	    centIndex[level][radius]++;
	    newData.setCenter(level,radius,centerIndex,newCenter);
	    centerParam = centerParam.nextSiblingElement();
	  }
	}
      }
    }
    int currentMaxLevel = 0;
    int currentMaxRadius = 0;
    int maxCenters = 0;
    for(int level = 0; level < 15; level++) {
      int levelSum = 0;
      for(int rad = 0; rad < 30; rad++) {
	levelSum += centIndex[level][rad]; 
	if((centIndex[level][rad] > 0)&&(rad > currentMaxRadius)) {
	  currentMaxRadius = rad;
	}
	if(centIndex[level][rad] > maxCenters)
	  maxCenters = centIndex[level][rad];
      }
      if((levelSum > 0) && (level > currentMaxLevel)) {
	currentMaxLevel = level;
      }
    }

    // Set the number centers collected on which levels and radii so the
    // get functions in SimplexData will be able to iterate correctly

    if(currentMaxLevel+1 < newData.getMaxLevels())
      newData.setNumLevels(currentMaxLevel+1);
    else
      newData.setNumLevels(newData.getMaxLevels());
    
    if(currentMaxRadius+1 < newData.getMaxRadii())
      newData.setNumRadii(currentMaxRadius+1);
    else 
      newData.setNumRadii(newData.getMaxRadii());
      
    if(maxCenters < newData.getMaxCenters())
      newData.setNumCenters(maxCenters);
    else
      newData.setNumCenters(newData.getMaxCenters());
    
    QList<SimplexData>::append(newData);
    simplexDataConfigs->append(newConfig);
    configFileNames->append(nodeFileName);
    //newData.printString();
    return true;
  }
  return false;
}

void SimplexList::setFileName(const QString &newFileName)
{
  fileName = newFileName;
}

void SimplexList::setRadarName(const QString &newRadarName)
{
	radarName = newRadarName;
	QDomElement header = config->getConfig("simplex");
	config->setParam(header, "radar", radarName);

}

void SimplexList::setVortexName(const QString &newVortexName)
{
	vortexName = newVortexName;
	QDomElement header = config->getConfig("simplex");
	config->setParam(header, "name", vortexName);

}

void SimplexList::setNewWorkingDirectory(const QString &newDirectory)
{
  workingDir = newDirectory;
}

void SimplexList::createDomSimplexDataEntry(const SimplexData &newData)

  /* 
   * This is used to keep the configuration Dom tree consistant
   *   with the simplexData stored in the list. This function commits 
   *   non-null information from the simplexData to the appropriate 
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
  
  QString defaultFile = QString("vortrac_defaultSimplexListStorage.xml");
  
  //QString nodeFile = vortexName+"simplex"+timeString+QString(".xml");
  //  QFile fileCheck(workingDir+nodeFile);
  //if(QFile::exists(workingDir+nodeFile))
  QString nodeFile = vortexName+"_"+radarName+"simplex"+timeString+QString(".xml");

  Configuration *newConfig = new Configuration(0,defaultFile);

  config->addDom(parent, QString("time"), 
		 newData.getTime().toString(Qt::ISODate)); 
  config->addDom(parent, QString("file"), workingDir+nodeFile);

  simplexDataConfigs->append(newConfig);
  configFileNames->append(workingDir+nodeFile);

  // Add this information to the new catelog entry

  QDomElement volRoot = newConfig->getRoot();
  QDomElement header = newConfig->getConfig("simplex");
  newConfig->setParam(header, "name", vortexName);
  newConfig->setParam(header, "radar", radarName);
  newConfig->setParam(header, "product", "simplex");

  newConfig->addDom(header, "time", newData.getTime().toString(Qt::ISODate));
  newConfig->addDom(header, "numpoints", 
		      QString().setNum(newData.getNumPointsUsed()));
  
  for(int i = 0; i < newData.getNumLevels(); i++) {
    for(int j = 0; j < newData.getNumRadii(); j++) {
      QString levelIndex = QString().setNum(i);
      QString radiusIndex = QString().setNum(j);
      if(!newData.emptyLevelRadius(i,j)) {
	newConfig->addDom(volRoot, QString("sum"+levelIndex+radiusIndex), 
			  QString(""));
	QDomElement volParent =newConfig->getConfig(QString("sum"+levelIndex+radiusIndex));
	volParent.setAttribute(QString("radius"),radiusIndex);
	volParent.setAttribute(QString("level"), levelIndex);
	if(newData.getX(i,j)!=-999) 
	  newConfig->addDom(volParent, QString("meanx"), 
			    QString().setNum(newData.getX(i,j)));
	if(newData.getY(i,j)!=-999) 
	  newConfig->addDom(volParent,  QString("meany"), 
			    QString().setNum(newData.getY(i,j)));
	if(newData.getHeight(i)!=-999) 
	  newConfig->addDom(volParent,  QString("height"), 
			    QString().setNum(newData.getHeight(i)));
	if(newData.getRadius(i)!=-999) 
		newConfig->addDom(volParent,  QString("radius"), 
						  QString().setNum(newData.getRadius(j)));
	if(newData.getCenterStdDev(i,j)!=-999) 
	  newConfig->addDom(volParent,  QString("center_stddev"), 
			    QString().setNum(newData.getCenterStdDev(i,j)));
	if(newData.getMaxVT(i,j)!=-999) 
	  newConfig->addDom(volParent, QString("meanvt"), 
			    QString().setNum(newData.getMaxVT(i,j)));
	if(newData.getVTUncertainty(i,j)!=-999) 
	  newConfig->addDom(volParent,  QString("meanvt_stddev"), 
			    QString().setNum(newData.getVTUncertainty(i,j)));
	if(newData.getNumConvergingCenters(i,j)!=-999)
	  newConfig->addDom(volParent, QString("numcenters"),
		       QString().setNum(newData.getNumConvergingCenters(i,j)));
	
	bool hasCenters  = false;
	for(int k = 0; k < newData.getNumCenters(); k++) {
	  if(!newData.getCenter(i,j,k).isNull())
	    hasCenters = true;
	}

	if(hasCenters) {
	  newConfig->addDom(volRoot, QString("search"+levelIndex+radiusIndex),
			    QString(""));
	  QDomElement searchParent = newConfig->getConfig(QString("search"+levelIndex+radiusIndex));
	  searchParent.setAttribute(QString("radius"), radiusIndex);
	  searchParent.setAttribute(QString("level"), levelIndex);
	  
	  for(int k = 0; k < newData.getNumCenters(); k++) {
	    if(newData.getCenter(i,j,k).isNull())
	      continue;

	    QString centerIndex = QString().setNum(k);
	    Center currCenter = newData.getCenter(i,j,k);
	    
	    if(currCenter.getX()!=-999) {
	      newConfig->addDom(searchParent,  QString("x"),  
				QString().setNum(currCenter.getX()), 
				QString("center"), centerIndex);
	      if(newData.getInitialX(i,j,k)!=-999) {
		QDomElement xElem = newConfig->getElementWithAttrib(searchParent, QString("x"),QString("center"), centerIndex);
		xElem.setAttribute(QString("x0"), QString().setNum(newData.getInitialX(i,j,k)));
	      }
	    }	    
	    if(currCenter.getY()!=-999) {
	      newConfig->addDom(searchParent,  QString("y"), 
				QString().setNum(currCenter.getY()),
				QString("center"), centerIndex);
	      if(newData.getInitialY(i,j,k)!=-999) {
		QDomElement yElem = newConfig->getElementWithAttrib(searchParent, QString("y"),QString("center"), centerIndex);
		yElem.setAttribute(QString("y0"), QString().setNum(newData.getInitialY(i,j,k)));
	      }
	    }
	    if(currCenter.getMaxVT()!=-999)
	      newConfig->addDom(searchParent,  QString("maxvt"), 
				QString().setNum(currCenter.getMaxVT()),
				QString("center"), centerIndex);
	  }
	}
      }
    }
  }
}

     
void SimplexList::append(const SimplexData &value)
{
  createDomSimplexDataEntry(value);
  QList<SimplexData>::append(value);
}

void SimplexList::removeAt(int i)
{
  int initialCount = simplexDataConfigs->count();
    configFileNames->removeAt(i);
  simplexDataConfigs->removeAt(i);
  if(simplexDataConfigs->count() >= initialCount)
    Message::toScreen("SimplexList: Remove At didn't get smaller");
  QString timeString = this->at(i).getTime().toString(Qt::ISODate);
  timeString.replace(QString("-"), QString("_"));
  timeString.replace(QString(":"), QString("_"));
 
  // Remove element and children from main xml identifing specific file

  QDomElement parent = config->getConfig(QString("volume_"+timeString));
  if(parent.tagName()!=QString("volume_"+timeString))
    Message::toScreen("The tag names are different parent: "+parent.tagName()+" what we got "+QString("volume_"+timeString));
  config->removeDom(parent, QString("time")); 
  config->removeDom(parent, QString("file"));
  QDomElement root = config->getRoot();
  config->removeDom(root, QString("volume_"+timeString));

  QList<SimplexData>::removeAt(i);
}

void SimplexList::timeSort()
{
  for(int i = 0; i < this->count(); i++) {
    for(int j = i+1; j < this->count(); j++) {
      if(this->at(i).getTime() > this->at(j).getTime()) {
	this->swap(j,i);
	simplexDataConfigs->swap(j,i);
	configFileNames->swap(j,i);
      }
    }
  }
  
}

int SimplexList::getLatestIndex()
{
  QDateTime startTime = this->at(0).getTime();
  int timeRefIndex = 0;
  for(int i = 1; i < this->count(); i++) {
    if(this->at(i).getTime() > startTime) {
      timeRefIndex = i;
      startTime = this->at(i).getTime();
    }
  }

  return timeRefIndex;
}
