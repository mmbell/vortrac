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
}


bool SimplexList::save()
{
  if(config->write(fileName)) {
    int fileSaves = 0;
    for(int i = 0; i < simplexDataConfigs->count(); i++) {
      Configuration *currConfig= simplexDataConfigs->value(i);
      if(currConfig->write(configFileNames->value(i)))
	fileSaves++;
      Message::toScreen(QString().setNum(fileSaves));
    }
    if(fileSaves == QList<SimplexData>::count())
      return true;
  }

  Message::toScreen("SIMPLEXLIST FAIL:Failed to save to file");
  return false;
}

bool SimplexList::open()
{

  int numConfigsOpened = 0;
  Message::toScreen("!num in Config groupList ="+QString().setNum(config->getGroupList()->count()));

  for(int i = 0; i < config->getGroupList()->count(); i++) {

    QDomNode node = config->getGroupList()->item(i);
    QDomElement currElement = node.toElement();
    if(!currElement.tagName().startsWith(QString("vol"))) {
      // Simplex Node General Parameters
      vortexName = config->getParam(currElement, "name");
      radarName = config->getParam(currElement, "radar");
      productType = config->getParam(currElement, "product");
      numConfigsOpened++;
      Message::toScreen("found the header config for index");
    }
    else {
      // Has information about a simplexData
      Message::toScreen("found data node");
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
    QDateTime nodeTime = QDateTime::fromString(nodeTimeString,Qt::ISODate);
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
      Message::toScreen("child Element = "+childElement.tagName());
      QString newName, newRadar, newType, newTime;
      if(childElement.tagName().startsWith(QString("simplex"))) {
	newName = newConfig->getParam(childElement, "name");
	newRadar = newConfig->getParam(childElement, "radar");
	newType = newConfig->getParam(childElement, "product");
	newTime = newConfig->getParam(childElement, "time");
	newData.setTime(QDateTime::fromString(newTime, Qt::ISODate));
	newData.setNumPointsUsed(newConfig->getParam(childElement, 
						     "numpoints").toInt());
	// Check to make sure these line up........
      }
      else {
	int level = childElement.attribute("level").toInt();
	int radius = childElement.attribute("radius").toInt();
	if(childElement.tagName().startsWith(QString("sum"))) {
	  // Sum Node average parameters
	  newData.setX(level, radius, 
		       config->getParam(childElement, 
					QString("meanx")).toFloat());
	  newData.setY(level, radius, 
		       config->getParam(childElement, "meany").toFloat());
	  newData.setHeight(level,
			   config->getParam(childElement, "height").toFloat());
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
	  
	  // Has information about a center
	  QDomElement centerParam = childElement.firstChildElement();
	  while(!centerParam.isNull()) {
	    Center newCenter;
	    int centerIndex = centerParam.attribute("center").toInt();
	    newCenter.setLevel(level);
	    newCenter.setRadius(radius);
	    newCenter.setX(newConfig->getParam(childElement, "x").toFloat());
	    newCenter.setY(newConfig->getParam(childElement, "y").toFloat());
	    newCenter.setMaxVT(newConfig->getParam(childElement, 
						"maxvt").toFloat());
	    centIndex[level][radius]++;
	    newData.setCenter(level,radius,centerIndex,newCenter);
	    centerParam = centerParam.nextSiblingElement();
	  }
	}
      }
    }
    QList<SimplexData>::append(newData);
    simplexDataConfigs->append(newConfig);
    configFileNames->append(nodeFileName);
    return true;
  }
  return false;
}

void SimplexList::setFileName(const QString &newFileName)
{
  fileName = newFileName;
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
  
  QString nodeFile = vortexName+"simplex"+timeString+QString(".xml");
  //  QFile fileCheck(workingDir+nodeFile);
  if(QFile::exists(workingDir+nodeFile))
    nodeFile = vortexName+"_"+radarName+"simplex"+timeString;

  Configuration *newConfig = new Configuration(0,workingDir+defaultFile);

  config->addDom(parent, QString("time"), 
		 newData.getTime().toString(Qt::ISODate)); 
  config->addDom(parent, QString("file"), workingDir+nodeFile);

  simplexDataConfigs->append( newConfig);
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
	
	bool hasCenters = false;
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
	    QString centerIndex = QString().setNum(k);
	    Center currCenter = newData.getCenter(i,j,k);
	    
	    if(currCenter.getX()!=-999)
	      newConfig->addDom(searchParent,  QString("x"),  
				QString().setNum(currCenter.getX()), 
				QString("center"), centerIndex);
	    if(currCenter.getY()!=-999)
	      newConfig->addDom(searchParent,  QString("y"), 
				QString().setNum(currCenter.getY()),
				QString("center"), centerIndex);
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

void SimplexList::timeSort()
{
  for(int i = 0; i < this->count(); i++) {
    for(int j = 0; j < this->count()-1; j++) {
      if(this->value(j)>this->value(j+1)) {
	this->swap(j+1,j);
	simplexDataConfigs->swap(j+1,j);
	configFileNames->swap(j+1,j);
      }
    }
  }
  
}