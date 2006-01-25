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


Configuration::Configuration(QWidget *parent, const QString &filename)
  :QWidget(parent)
{
  if(filename.isEmpty()) 
    {
      // Create a new configuration from scratch
      QDomDocument domDoc("CurrConfig");
      /* if(!read("vortrac_default.xml")) {
	 emit log(Message("Error reading configuration file (default)"));
	 } */
    }
  else
    {
      QDomDocument domDoc("CurrConfig");
      if(!read(filename)) {
	emit log(Message("Error reading configuration file (constructor)"));
      }
    }
  isModified = false;
}

Configuration::~Configuration()
{
}

QDomElement Configuration::getRoot()
{
  return domDoc.documentElement();
}

bool Configuration::read(const QString &filename)
{
  // Read in an existing configuration from an XML file
  
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    emit log(Message("Error opening configuration file"));
    return false;
  }

  QString errorStr;
  int errorLine;
  int errorColumn;
  

  // Set the DOM document contents to those of the file
  if (!domDoc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
    QString errorReport = QString("Parse error at line %1, column %2:\n%3")
      .arg(errorLine)
      .arg(errorColumn)
      .arg(errorStr);
    emit log(Message(errorReport));
    file.close();
    return false;
  }
  file.close();

  // Basic check to see if this is really a configuration file
  QDomElement root = domDoc.documentElement();
  if (root.tagName() != "vortrac") {
      emit log(Message("The file is not an VORTRAC configuration file."));
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
  

}

bool Configuration::write(const QString &filename)
{
  // Write out the DOM tree to an XML file
  const int IndentSize = 4;
 
  QFile file(filename);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    // return false;
    QTextStream out(&file);
    domDoc.save(out, IndentSize);
    isModified = false;
    return true;
  }
  return false;
}

QDomElement Configuration::getConfig(const QString &configName)
{

  QDomElement child = groupList.item(indexForTagName.value(configName)).toElement();
  if (child.isNull()) {
    emit log(Message("Null Node!"));
  }

  return child;

}

const QString Configuration::getParam(const QDomElement &element,
				      const QString &paramName)
{
  QString paramValue = element.firstChildElement(paramName).text();
  return paramValue;
}

void Configuration::setParam(const QDomElement &element,
			     const QString &paramName,
			     const QString &paramValue)
{
  // Change a value in the DOM tree
  QDomElement oldElement = element.firstChildElement(paramName);
  QDomElement newElement = domDoc.createElement(paramName);

  QDomText newText = domDoc.createTextNode(paramValue);
  newElement.appendChild(newText);

  oldElement.parentNode().replaceChild(newElement, oldElement);

  // Note that we modified the dom tree
  isModified = true;
  
  QString message("Configuration Changed: ");
  message+=(element.tagName()+": "+paramName+" = "+paramValue);
  emit log(Message(message));
  emit configChanged();

}

void Configuration::addDom(const QDomElement &element, 
			   const QString &paramName, 
			   const QString &paramValue)
{
  QString siblingName(paramName);
  siblingName = siblingName.remove("maxdatagap_");
  int last = siblingName.toInt();
  siblingName = QString("maxdatagap_"+QString().setNum(last-1));

  QDomNode newChild(element.firstChildElement(siblingName).cloneNode());
  QDomElement newChildElement = newChild.toElement();
  newChildElement.setTagName(paramName);
  int index = indexForTagName[element.tagName()];
  QDomNode parentNode = groupList.item(index);
  parentNode.insertAfter(newChildElement, 
			 element.lastChildElement(siblingName));
  setParam(element, paramName, paramValue);
  emit configChanged();
}

void Configuration::removeDom(const QDomElement &element,
			      const QString &paramName)
{
  QDomElement oldChild(element.firstChildElement(paramName));
  int index = indexForTagName[element.tagName()];
  QDomNode testNode(groupList.item(index).removeChild(oldChild));
  if (testNode.isNull())
    emit log(Message("Unable to delete the dom element"));
  emit configChanged();
}

void Configuration::catchLog(const Message& message)
{
  emit log(message);
}

