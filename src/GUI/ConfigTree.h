/*
 *
 * ConfigTree.h
 *
 * Created by Michael Bell on 7/21/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef CONFIGTREE_H
#define CONFIGTREE_H

#include <QDomDocument>
#include <QHash>
#include <QIcon>
#include <QTreeWidget>
#include "Config/Configuration.h"

class ConfigTree : public QTreeWidget
{
  Q_OBJECT
    
 public:
   ConfigTree(QWidget *parent = 0, Configuration *initialConfig = 0);
   bool read();
   // bool write(const QString &fileName);

 public slots:
   void reread();
   void catchLog(const Message& message);

 signals:
   void log(const Message& message);

   void newParam(const QDomElement &element,
		 const QString &paramName, 
		 const QString &paramValue);

   void addDom(const QDomElement &element, 
	       const QString &paramName, 
	       const QString &paramValue);
   // Sends information needed to create a new dom element

   void removeDom(const QDomElement &element, const QString &name);
   // Sends information needed to remove a dom element that is no longer needed

 private slots:
   void updateDomElement(QTreeWidgetItem *item, int column);

 private:
   void parseDomElement(const QDomElement &element,
			QTreeWidgetItem *parentItem = 0);
   QTreeWidgetItem *createItem(const QDomElement &element,
			       QTreeWidgetItem *parentItem = 0);
   void updateDataGaps(QDomElement element, 
		       QTreeWidgetItem *parentItem, QString oldValue);
   
   QHash<QTreeWidgetItem *, QDomElement> domElementForItem;
   QIcon folderIcon;
   QIcon bookmarkIcon;
   Configuration *configData; 

};

#endif
