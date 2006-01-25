/*
 *  Configuration.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/6/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QDomDocument>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QHash>
#include "Message.h"

class Configuration:public QWidget
{

  Q_OBJECT
 
 public:
  Configuration(QWidget *parent = 0,const QString &filename = QString());
  ~Configuration();
  QDomElement getRoot();
  bool read(const QString &filename);
  bool write(const QString &filename);
  QDomElement getConfig(const QString &configName);
  const QString getParam(const QDomElement &element,
			 const QString &paramName);
  bool checkModified() { return isModified; }
  
 public slots:
   void catchLog(const Message& message);
   void setParam(const QDomElement &element,
		  const QString &paramName, 
		  const QString &paramValue);
  void addDom(const QDomElement &element,
	      const QString &paramName,
	      const QString &paramValue);
  void removeDom(const QDomElement &element,
		 const QString &paramName);

 private:
  QDomDocument domDoc;
  QDomElement root;
  QDomNodeList groupList;
  QHash<QString, int> indexForTagName;
  bool isModified;


 signals:
  void log(const Message& message);
  void configChanged();
  //  void configChanged(const QDomElement &element);
};

#endif
