/*
 *  Configuration.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/6/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "Configuration.h"

#include <QDomText>


Configuration::Configuration(QObject *parent, const QString &filename)
  :QObject(parent)
{
  this->setObjectName("configuration");
  logChanges = true;
  if(filename.isEmpty()) 
    {
      // Create a new configuration from scratch
      QDomDocument domDoc("CurrConfig");
      /*      if(!read("vortrac_default.xml")) {
	 emit log(Message(QString("Error Reading Default Configuration File, Please Locate vortrac_default.xml"),0,this->objectName(),Red,QString("Locate Default File")));
	 } */
    }
  else
    {
      QDomDocument domDoc("CurrConfig");
      if(!read(filename)) {
	emit log(Message(QString("Error Reading Configuration File "+filename),0,this->objectName(),Red,QString("Could Not Locate File")));
      }
    }
  isModified = false;
  emit log(Message(QString(),0,this->objectName(),Green));
}

Configuration::~Configuration()
{
  domDoc.clear();
  // Clears out contents of domDoc including anything that is dynamically
  // created
}

QDomElement Configuration::getRoot()
{
  return domDoc.documentElement();
}

QDomNodeList* Configuration::getGroupList()
{
  return &groupList;
}

bool Configuration::read(const QString &filename)
{
  // Read in an existing configuration from an XML file

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    emit log(Message(QString("Error Opening Configuration File, Check Permissions on "+filename),0,this->objectName(),Red, QString("Check File Permissions")));
    return false;
  }

  QString errorStr;
  int errorLine;
  int errorColumn;
  

  // Set the DOM document contents to those of the file
  if (!domDoc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
    QString errorReport = QString("XML Parse Error in "+filename+" at Line %1, Column %2:\n%3")
      .arg(errorLine)
      .arg(errorColumn)
      .arg(errorStr);
    emit log(Message(errorReport,0,this->objectName(),Yellow, QString("XML Parse Error")));
    file.close();
    return false;
  }
  file.close();

  // Basic check to see if this is really a configuration file
  QDomElement root = domDoc.documentElement();
  if (root.tagName() != "vortrac") {
      emit log(Message(QString("The file is not an VORTRAC configuration file."),0,this->objectName(),Red,QString("Not a VORTRAC file")));
    return false;

  }

  // Creat a hash of indices and tag names
  groupList = root.childNodes();
  for (int i = 0; i <= groupList.count()-1; i++) {
    QDomNode currNode = groupList.item(i);
    QDomElement group = currNode.toElement();
    indexForTagName.insert(group.tagName(), i);
  }
  isModified = false;
  return true;
  
  emit log(Message(QString(), 0, this->objectName(),Green));

}

bool Configuration::write(const QString &filename)
{
  // Write out the DOM tree to an XML file
  const int IndentSize = 4;
 
  QFile file(filename);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    domDoc.save(out, IndentSize);
    isModified = false;
    emit log(Message(QString(),0,this->objectName(),Green));
    return true;
  }
  emit log(Message(QString("Failed to save XML File to "+filename),0,this->objectName(),Yellow,QString("Failed to save XML file")));
  return false;
}

bool Configuration::isNull()
{
  if(!domDoc.isNull())
    return true;
  return false;
}

QDomElement Configuration::getConfig(const QString &configName)
{

  /*
   * Returns a config element with the specified tagName, if one does not exist
   *   a null element is returned. Config elements are first layer elements in 
   *   a configuration object, all other elements are simply elements. The names
   *   and indexes of all first layer elements (those that are direct children
   *   of the vortrac element) are kept in the indexForTagName hash.
   *
   */

  QDomElement child = groupList.item(indexForTagName.value(configName)).toElement();
  if (child.isNull()) {
    emit log(Message(QString("Null Config Node: "+configName),0,this->objectName(),Yellow, QString("Bad XML Node")));
  }
  if(child.tagName()!=configName){
    emit log(Message(QString("Could Not Find The Config Node "+configName),0,
		     this->objectName()));
    return QDomElement();
  }
  return child;

}

QDomElement Configuration::getConfig(const QString &configName,
				     const QString &attribName,
				     const QString &attribValue)
{

  /*
   * Returns a config element with the specified tagName, if one does not exist
   *   a null element is returned. Config elements are first layer elements in 
   *   a configuration object, all other elements are simply elements. The names
   *   and indexes of all first layer elements (those that are direct children
   *   of the vortrac element) are kept in the indexForTagName hash.
   *
   */

  QList<int> checkThese = indexForTagName.values(configName);
  for(int i = 0; i < checkThese.count(); i++) {
    QDomElement child = groupList.item(checkThese[i]).toElement();
    if(!child.isNull()) {
      if(child.attribute(attribName)==attribValue) {
	return child;
      }
    }
  }
  emit log(Message(QString("Null XML Node/Element: "+configName+"."+attribName+" = "+attribValue),0,this->objectName()));
  QDomElement child;
  return child;
}

const QString Configuration::getParam(const QDomElement &element,
				      const QString &paramName)
{
  /*
   * Returns the text value of an element with the name "paramName".
   *   If no such element exists it return an empty string, and emits an 
   *   error log entry describing the missing element.
   *
   */

  QDomElement elementWithParam = getElement(element, paramName);
  QString paramValue;
  if(!elementWithParam.isNull()) {
    paramValue = elementWithParam.text();
  }
  return paramValue;
}

const QString Configuration::getParam(const QDomElement &element,
				      const QString &paramName,
				      const QString &attribName,
				      const QString &attribValue)
{
  /*
   * This is an overloaded function to return the value of element named 
   *  "paramName" that also has a specified attribute name "attribName", 
   *   and attribute value "attribValue". An empty string and an error 
   *   are returned if this element does not exist.
   *
   */

  QDomElement elementWithAttrib = getElementWithAttrib(element, paramName, 
						       attribName, attribValue);
  QString paramValue;
  if(!elementWithAttrib.isNull()) {
    paramValue = elementWithAttrib.text();
  }
  return paramValue;

}

void Configuration::setParam(const QDomElement &element,
			     const QString &paramName,
			     const QString &paramValue)
{
  /*
   * This slot sets the value of an element with name "paramName" and parent
   *   "element" to value "paramValue".  It also emits a log signal indicating
   *   exactly how the configuration file was changed. If the specified element
   *   cannot be located within the DOM structure an error signal is sent.
   *
   */

  QDomElement oldElement = getElement(element, paramName);
  if(!oldElement.isNull()) {
    QDomElement newElement = domDoc.createElement(paramName);
    
    QDomText newText = domDoc.createTextNode(paramValue);
    newElement.appendChild(newText);
    
    oldElement.parentNode().replaceChild(newElement, oldElement);
    
    // Note that we modified the dom tree
    isModified = true;
    
    QString message("Configuration Changed: ");
    message+=(element.tagName()+": "+paramName+" = "+paramValue);
    if(logChanges)
      emit log(Message(message));
    emit configChanged();
  }
  else {
    addDom(element, paramName, paramValue);
  }
}

void Configuration::setParam(const QDomElement &element,
			     const QString &paramName,
			     const QString &paramValue,
			     const QString &attribName,
			     const QString &attribValue)
{
  /*
   * This overloaded slot sets the value of an element specified by  name 
   *   "paramName", parent "element", attribute name "attribName", and attribute
   *   value to "attribValue", and sets the text to value "paramValue".  It 
   *   also emits a log signal indicating exactly how the configuration 
   *   file was changed. If the element indicated cannot be found within the
   *   configuration DOM tree and error message signal is sent.
   *
   */

  QDomElement oldElement = getElementWithAttrib(element, paramName, 
						attribName, attribValue);
  if(!oldElement.isNull()) {
    QDomElement newElement = domDoc.createElement(paramName);
    newElement.setAttribute(attribName, attribValue);
    
    QDomText newText = domDoc.createTextNode(paramValue);
    newElement.appendChild(newText);
    
    oldElement.parentNode().replaceChild(newElement, oldElement);
    
    // Note that we modified the dom tree
    isModified = true;
    
    QString message("Configuration Changed: ");
    message+=(element.tagName()+": "+paramName+": "+attribName+": "+attribValue);
    message+=(" = "+paramValue);
    if(logChanges)
      emit log(Message(message));
    emit configChanged();
  }
}

void Configuration::addDom(const QDomElement &element, 
			   const QString &paramName, 
			   const QString &paramValue)
{
  /* 
   * This slot adds an item to the DOM structure. This slot can be used to add
   *   either a regular element, or a new config element, depending on the 
   *   parent element indicated. If a new config is made, a generic element is
   *   as it child to hold the place for latter placement on children elements.
   *   If the parent element cannot be found an error signal is returned.
   *
   */

  if(element.isNull()) {
    emit log(Message(QString("Could Not Find Config Section "+element.tagName()),0,this->objectName()));
  }
  else {
    QDomElement newChildElement = domDoc.createElement(paramName);
    QDomNode parentNode;
    if(indexForTagName.contains(element.tagName())){
      int index = indexForTagName[element.tagName()];
      parentNode = groupList.item(index);
      newChildElement.appendChild(domDoc.createTextNode(paramValue));
      QDomElement empty = element.firstChildElement("EMPTY");
      if(empty.tagName()==QString("EMPTY")) {
	parentNode.replaceChild(newChildElement, empty);
      }
      else {
	parentNode.appendChild(newChildElement);
      }
      QString message("Config Changed: ");
      message+=(element.tagName()+": "+paramName+"= "+paramValue+" was created");
      if(logChanges)
	emit log(Message(message));
    }
    else {
      parentNode = getRoot(); 
      QDomDocumentFragment temp = domDoc.createDocumentFragment();
      QDomElement empty = domDoc.createElement("EMPTY");
      empty.appendChild(domDoc.createTextNode("replace"));
      temp.appendChild(empty);
      newChildElement.appendChild(temp);
      parentNode.appendChild(newChildElement);
      indexForTagName.insert(newChildElement.tagName(), 
			     indexForTagName.count());

      QString message("Config Changed: ");
      message+=(element.tagName()+": "+paramName+" was created");
      if(logChanges)
	emit log(Message(message));
    }
    
    isModified = true;
    emit configChanged();
  }
}

void Configuration::addDom(const QDomElement &element,
			   const QString &paramName,
			   const QString &paramValue,
			   const QString &attribName,
			   const QString &attribValue)
{

  /* 
   * This slot is an overloaded slot that adds an element with an attribute
   *   to the DOM structure. This slot can be used to add either a regular 
   *   element with an attribute, or a new config element with an attribute, 
   *   depending on the parent element indicated. If a new config is made, 
   *   a generic element is as it child to hold the place for latter 
   *   placement on children elements. If the parent element cannot be found 
   *   an error signal is returned.
   *
   */

  if(element.isNull()) {
    emit log(Message(QString("Could Not Find Config Section "+element.tagName()),0,this->objectName()));
  }
  else {
    QDomElement newChildElement;
    
    newChildElement = domDoc.createElement(paramName);
    newChildElement.setAttribute(attribName, attribValue);
    
    QDomNode parentNode;
    if(indexForTagName.contains(element.tagName())){
      int index = indexForTagName[element.tagName()];
      parentNode = groupList.item(index);
      newChildElement.appendChild(domDoc.createTextNode(paramValue));
      QDomElement empty = element.firstChildElement("EMPTY");
      if(empty.tagName()==QString("EMPTY")) {
	parentNode.replaceChild(newChildElement, empty);
      }
      else {
	parentNode.appendChild(newChildElement);
      }

      QString message("Config Changed: ");
      message+=(element.tagName()+": "+paramName+": "+attribName+": ");
      message+=(attribValue+" = "+paramValue+" was created");
      if(logChanges)
	emit log(Message(message));
    }
    else{
      parentNode = getRoot(); 
      QDomDocumentFragment temp = domDoc.createDocumentFragment();
      QDomElement empty = domDoc.createElement("EMPTY");
      empty.appendChild(domDoc.createTextNode("replace"));
      temp.appendChild(empty);
      newChildElement.appendChild(temp);
      parentNode.appendChild(newChildElement);
      indexForTagName.insert(newChildElement.tagName(), 
			     indexForTagName.count());

      QString message("Config Changed: ");
      message+=(element.tagName()+": "+paramName+": "+attribName+": ");
      message+=(attribValue+" was created");
      if(logChanges)
	emit log(Message(message));
    }
      
      isModified = true;
      emit configChanged();
  }
  
}

void Configuration::removeDom(const QDomElement &element,
			      const QString &paramName)
{

  /*
   * This slot removes the element specified by name "paramName" and parent
   *   element "element" from the DOM structure. If the desired element can
   *   not be located an error signal is released.
   *
   */
  
  QDomElement oldElement = getElement(element, paramName);
  if(!oldElement.isNull()) {
    int index = indexForTagName[element.tagName()];
    QDomNode testNode(groupList.item(index).removeChild(oldElement));
    if (testNode.isNull())
      emit log(Message(QString("Unable to delete the element "+paramName+" from "+element.tagName()),0,this->objectName()));

    QString message("Config Changed: ");
    message+=(element.tagName()+": "+paramName+" was removed");
    if(logChanges)
      emit log(Message(message,0,this->objectName()));
    emit configChanged();
  }
}

void Configuration::removeDom(const QDomElement &element,
			      const QString &paramName,
			      const QString &attribName,
			      const QString &attribValue)
{

  /*
   * This slot is an overloaded slot designed to remove the element specified 
   *   by name "paramName", with and attributed named "attribName", that has 
   *   a value of "attribValue", and parent element "element" from the DOM 
   *   structure. If the desired element can not be located an error 
   *   signal is released.
   *
   */

  QDomElement oldElement = getElementWithAttrib(element, paramName, attribName,
						attribValue);
  if(!oldElement.isNull()) {
    int index = indexForTagName[element.tagName()];
    QDomNode testNode(groupList.item(index).removeChild(oldElement));
    if (testNode.isNull())
      emit log(Message(QString("Unable to delete the element "+paramName+" from "+element.tagName()),0,this->objectName()));

    QString message("Config Changed: ");
    message+=(element.tagName()+": "+paramName+": "+attribName+": ");
    message+=(attribValue+" was removed");
    if(logChanges)
      emit log(Message(message));

    emit configChanged();
  }
}
 
const QString Configuration::getAttribute(const QDomElement &element,
				    const QString &paramName,
				    const QString &attribName)
{
  /*
   * This slot returns the value of the attribute named "attribName" 
   *   belonging to the element specified by name "paramName" in parent
   *   element "element". If the correct element and attribute can not be 
   *   located within the DOM structure and empty string is returned and an
   *   error signal is emitted.
   *
   */

  QDomElement elementWithParam = getElement(element, paramName); 
  QString attribValue;
  while(!elementWithParam.isNull()
	&& attribValue.isEmpty()
	&& (elementWithParam.tagName()==paramName)) {
    if(elementWithParam.hasAttribute(attribName)) {
      attribValue = elementWithParam.attribute(attribName);
    }
    elementWithParam.nextSiblingElement(paramName);
  }
  if(attribValue.isNull()) {
    QString errMessage = "Could Not Find The Config Element "+element.tagName();
      errMessage +=(": " + paramName + ": " + attribName);
      emit log(Message(errMessage,0,this->objectName()));
  }
  return attribValue;
}


void Configuration::setAttribute(const QDomElement &element,
				 const QString &paramName,
				 const QString &attribName,
				 const QString &attribValue)
{
  QDomElement elementWithParam = getElement(element, paramName);
  elementWithParam.setAttribute(attribName, attribValue);
  if((elementWithParam.hasAttribute(attribName))
     &&(elementWithParam.attribute(attribName)==attribValue)) 
    return;
  
  QString failMessage = QString("Failed to add the attribute "+attribName+" with value "+attribValue+" onto element "+paramName+" within the config "+getRoot().tagName());
  emit log(Message(failMessage, 0, this->objectName()));
}

void Configuration::catchLog(const Message& message)
{
  emit log(message);
}

const QDomElement Configuration::getElementWithAttrib(const QDomElement &element,
						      const QString &paramName,
						      const QString &attribName,
						      const QString &attribValue)
{

  /*
   * Private function used to insure that the element retrieved is the 
   *   desired element, with the correct attribute and attribute value.
   *
   */

  QDomElement elementWithAttribute;
  elementWithAttribute.clear();
  QDomElement sibling = getElement(element, paramName);
  if(!sibling.isNull()){
    while((!sibling.isNull()) && (sibling.tagName()==paramName)) {
      if(sibling.hasAttribute(attribName)) {
	if(sibling.attribute(attribName)==attribValue) {
	  elementWithAttribute = sibling;
	}
      }
      sibling = sibling.nextSiblingElement(paramName);
    }
    if(elementWithAttribute.isNull()) {
      QString errMessage = "Could Not Find The Config Element "+element.tagName();
      errMessage +=(": " + paramName + ": " + attribName + ": " + attribValue);
      emit log(Message(errMessage,0,this->objectName()));
      
    }
  }
  return elementWithAttribute;
}

const QDomElement Configuration::getElement(const QDomElement &element,
					    const QString &paramName)
{

  /*
   * Private function used to insure that the element retreived is the desired 
   *   element, if it is not an error signal is sent and a null element returned
   *
   */

  QDomElement desiredElement = element.firstChildElement(paramName);

  if((desiredElement.tagName()!=paramName)
     ||(desiredElement.isNull())) {

    QString errMessage = "Could Not Find Config Element: "+element.tagName();
    errMessage +=(": " + paramName);
    emit log(Message(errMessage,0,this->objectName()));
    
    desiredElement = QDomElement();
    desiredElement.clear();
  }
  
  return desiredElement;

}

Configuration& Configuration::operator = (const Configuration &other)
{
  domDoc = other.domDoc;
  root = other.root;
  groupList = other.groupList;
  indexForTagName = other.indexForTagName;
  isModified = other.isModified;
  return *this;
  
}

void Configuration::setLogChanges(bool logStatus)
{
  QString message;
  if(!logChanges) {
    message = QString(tr("Stopped Logging XML Item Change Messages For This Configuration"));
    emit log(Message(message, 0, this->objectName(),Green));
    logChanges = logStatus;
  }
  else {
    logChanges = logStatus;
    message = QString(tr("Started Logging XML Item Change Messages For This Configuration"));
    emit log(Message(message, 0, this->objectName(),Green));
  }
}
