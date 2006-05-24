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
  if(fileName.isEmpty())
    fileName = QString("vortrac_defaultVortexDataStorage.xml");

  config = new Configuration(0, newFileName);
  config->read(fileName);
  QList<VortexData>::QList<VortexData>();
}

VortexList::~VortexList()
{
}


bool VortexList::save()
{
  if(config->write(fileName))
    return true;

  Message::toScreen("Failed to save to file");
  return false;
}

bool VortexList::open()
{
  if(config->read(fileName)) {
    for(int i = 0; i < config->getGroupList()->count(); i++) {
      VortexData newData;
      QDomNode node = config->getGroupList()->item(i);
      if(!node.toElement().tagName().startsWith(QString("volume"))) {
	// Radar and Hurricane information kept here
      }
      else {
	
	// Has information about a vortexData;
	QDomElement currElement = node.toElement();
	QString newTimeString = config->getParam(currElement, "time");
	QDateTime newTime = QDateTime::fromString(newTimeString);
	newData.setTime(newTime);
	float newPressure = config->getParam(currElement,
					     "pressure").toFloat();
	newData.setPressure(newPressure);
	float newPressureUncertainty = config->getParam(currElement,
					     "pressure_uncertainty").toFloat();
	newData.setPressureUncertainty(newPressureUncertainty);
     
	for(int n = 0; n < newData.getNumLevels(); n++) {
	  QString nString = QString().setNum(n);
	  QString level("level");
	  
	  float newLat = config->getParam(currElement, "latitude", level,
					  nString).toFloat();
	  newData.setLat(n, newLat);
	  
	  float newLon = config->getParam(currElement, "longitude", level,
					  nString).toFloat();
	  newData.setLon(n, newLon);

	  float newAlt = config->getParam(currElement, "altitude", level,
					  nString).toFloat();
	  newData.setAltitude(n, newAlt);

	  float newRmw = config->getParam(currElement, "rmw", level,
					  nString).toFloat();
	  newData.setRMW(n, newRmw);
	  
	  float newRmwUncertainty = config->getParam(currElement, 
						     "rmw_uncertainty", 
						     level, 
						     nString).toFloat();
	  newData.setRMWUncertainty(n, newRmwUncertainty);
	  
	  int numCenters = config->getParam(currElement, 
					    "num_converging_centers",level,
					    QString().setNum(n)).toInt();
	  newData.setNumConvergingCenters(n, numCenters);

	  float centerStd = config->getParam(currElement, 
					     "center_std_dev",
					     level, nString).toFloat();
	  newData.setCenterStdDev(n, centerStd);

	  QDomElement coeff = currElement.firstChildElement("fourier");
	  while(!coeff.isNull()) {
	    Coefficient newCoeff;
	    newCoeff.setLevel(coeff.attribute(level).toInt());
	    newCoeff.setRadius(coeff.attribute("radius").toInt());
	    QString code = coeff.attribute("coefficient");
	    int num = QString(code[3]).toInt();
	    if((code[2]=='c')||(code[2]=='C')) {
	      newCoeff.setParameter("c");
	      switch(num)
		{
		case 0: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(0)); break;
		case 1: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(1)); break;
		case 2: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(2)); break;
		case 3: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(3)); break;
		case 4: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(4)); break;
		case 5: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(5)); break;
		}
	    }
	    else {
	      newCoeff.setParameter("s");
	      switch(num)
		{
		case 1: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(1)); break;
		case 2: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(2)); break;
		case 3: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(3)); break;
		case 4: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(4)); break;
		case 5: newCoeff.setParameter(newCoeff.getParameter()
					      +QString().setNum(5)); break;
		}
	      num += 6;
	    } 
	    if((newCoeff.getRadius() < newData.getNumRadii())
	       &&(newCoeff.getLevel() < newData.getNumLevels())
	       &&(num < 11)) {
	      if(code.startsWith("vt", Qt::CaseInsensitive)) {
		newData.setTangential((int)newCoeff.getLevel(), 
				      (int)newCoeff.getRadius(), 
				      num, newCoeff);
	      }
	      if(code.startsWith("vr", Qt::CaseInsensitive)) {
		newData.setRadial((int)newCoeff.getLevel(),
				  (int)newCoeff.getRadius(),
				  num, newCoeff);
	      }
	      if(code.startsWith("dz", Qt::CaseInsensitive)) {
		newData.setReflectivity((int)newCoeff.getLevel(), 
					(int)newCoeff.getRadius(),
					num, newCoeff);
	      }
	    }
	    coeff = coeff.nextSiblingElement("fourier");
	  }
	}
      }
    }
  }
  return false;
}

void VortexList::setFileName(const QString &newFileName)
{
  fileName = newFileName;
}

void VortexList::createDomVortexDataEntry(const VortexData& newData)
{
  QDomElement root = config->getRoot();
  config->addDom(root, QString("volume"), QString(""), QString("time"),
		 newData.getTime().toString());
  QDomElement parent = config->getConfig(QString("volume"), QString("time"), 
					 newData.getTime().toString());
  config->addDom(parent, QString("time"), newData.getTime().toString());
  config->addDom(parent, QString("pressure"), 
		 QString().setNum(newData.getPressure()));
  config->addDom(parent, QString("pressure_uncertainty"),
		 QString().setNum(newData.getPressureUncertainty()));
  for(int i = 0; i < newData.getNumLevels(); i++) {
    QString ii = QString().setNum(i);
    QString level("level");
    config->addDom(parent, QString("latitude"), 
		   QString().setNum(newData.getLat(i)), level, ii);
    config->addDom(parent, QString("longitude"),
		   QString().setNum(newData.getLon(i)), level, ii);
    config->addDom(parent, QString("altitude"),
		   QString().setNum(newData.getAltitude(i)), level, ii);
    config->addDom(parent, QString("rmw"),
		   QString().setNum(newData.getRMW(i)), level, ii);
    config->addDom(parent, QString("rmw_uncertainty"), 
		   QString().setNum(newData.getRMWUncertainty(i)), level, ii);
    config->addDom(parent, QString("num_converging_centers"),
		   QString().setNum(newData.getNumConvergingCenters(i)),
		   level, ii);
    config->addDom(parent, QString("center_std_dev"),
		   QString().setNum(newData.getCenterStdDev(i)), level, ii);
    for(int r = 0; r < newData.getNumRadii(); r++) {
      for(int w = 0; w < 2*newData.getNumWaveNum(); w++) {
	QString ff("fourier");
	QString rr = QString().setNum(r);
	QString ww = QString().setNum(w);
	QString coeff("coefficient");
	Coefficient tangCoeff = newData.getTangential(i,r,w);
	config->addDom(parent, ff, QString().setNum(tangCoeff.getValue()), 
		       coeff, QString("VT"+tangCoeff.getParameter()));
        QDomElement tang = config->getElementWithAttrib(root, ff, coeff, 
		  QString("VT"+tangCoeff.getParameter()));
	tang.setAttribute(QString("radius"), rr);
	tang.setAttribute(level, ii);
	
	Coefficient radCoeff = newData.getRadial(i,r,w);
	config->addDom(parent, ff, QString().setNum(radCoeff.getValue()), 
		       coeff, QString("VR"+radCoeff.getParameter()));
	QDomElement rad = config->getElementWithAttrib(root, ff, coeff, 
		      QString("VR"+radCoeff.getParameter()));
	rad.setAttribute(QString("radius"), rr);
	rad.setAttribute(level, ii);
	
	Coefficient refCoeff = newData.getReflectivity(i,r,w);
	config->addDom(parent, ff, QString().setNum(refCoeff.getValue()), coeff, 
		     QString("DZ"+refCoeff.getParameter()));
	QDomElement ref = config->getElementWithAttrib(root, ff, coeff, 
		      QString("DZ"+refCoeff.getParameter()));
	ref.setAttribute(QString("radius"), rr);
	ref.setAttribute(level, ii);
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
      if(this->value(j)>this->value(j+1))
	this->swap(j+1,j);
    }
  }
  
}
