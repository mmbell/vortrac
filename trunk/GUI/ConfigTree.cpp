/*
 *
 * ConfigTree.cpp
 *
 * Created by Michael Bell on 7/21/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include <QtGui>

#include "ConfigTree.h"

ConfigTree::ConfigTree(QWidget *parent, Configuration *initialConfig)
  : QTreeWidget(parent)
{

  // Set the current configuration
  configData = initialConfig;

  QStringList labels;
  labels << tr("Category") << tr("Parameter");
  
  header()->setResizeMode(QHeaderView::Stretch);
  setHeaderLabels(labels);
  
  folderIcon.addPixmap(style()->standardPixmap(QStyle::SP_DirClosedIcon),
		       QIcon::Normal, QIcon::Off);
  folderIcon.addPixmap(style()->standardPixmap(QStyle::SP_DirOpenIcon),
		       QIcon::Normal, QIcon::On);
  bookmarkIcon.addPixmap(style()->standardPixmap(QStyle::SP_FileIcon));

}

bool ConfigTree::read()
{ 
  clear();
  
  disconnect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
  	     this, SLOT(updateDomElement(QTreeWidgetItem *, int)));
  
  QDomNodeList nodeList = configData->getRoot().childNodes();
  for (int i = 0; i <= nodeList.count()-1; i++) {
    QDomNode currNode = nodeList.item(i);
    QDomElement child = currNode.toElement();
    parseDomElement(child);
  }

  connect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
  	  this, SLOT(updateDomElement(QTreeWidgetItem *, int)));
  
  return true;
}


void ConfigTree::updateDomElement(QTreeWidgetItem *item, int column)
{
  //QTreeWidgetItem *parentItem = item->parent();
  QDomElement element = domElementForItem[item];
  QDomElement configSection = element.parentNode().toElement();
  QString oldValue = element.text();
  if(oldValue!=item->text(column)) {
    emit newParam(configSection, element.tagName(),item->text(column));
    domElementForItem.insert(item,configSection.firstChildElement(element.tagName()));
    if (element.tagName() == QString("maxwavenumber")) {
      //      updateDataGaps(element, parentItem, oldValue);
    }
  }
}

void ConfigTree::parseDomElement(const QDomElement &element,
                                      QTreeWidgetItem *parentItem)
{
  QTreeWidgetItem *item = createItem(element, parentItem);

  QString paramName = element.tagName();
  if (paramName.isEmpty())
    paramName = QObject::tr("Unknown");

  item->setIcon(0, folderIcon);
  item->setText(0, paramName);

  QDomElement child = element.firstChildElement();
  while (!child.isNull()) {

    QTreeWidgetItem *childItem = createItem(child, item);
    QString paramName = child.tagName();
    if (child.hasAttribute("wave")) {
      paramName += "(";
      paramName += child.attribute("wave");
      paramName += ")";
    }
    
    childItem->setFlags(item->flags() | Qt::ItemIsEditable);
    childItem->setIcon(0, bookmarkIcon);
    childItem->setText(0, paramName);
    childItem->setText(1, child.text());
  
    child = child.nextSiblingElement();

  }
}

QTreeWidgetItem *ConfigTree::createItem(const QDomElement &element,
					QTreeWidgetItem *parentItem)
{
  QTreeWidgetItem *item;
  if (parentItem) {
    item = new QTreeWidgetItem(parentItem);
  } else {
    item = new QTreeWidgetItem(this);
  }
  domElementForItem.insert(item, element);
  return item;
}

void ConfigTree::reread()
{
  /*
  emit log(Message("In ConfigTree ReRead"));
  QList<QTreeWidgetItem *> items = domElementForItem.keys();
  for(int i = 0; i < items.count(); i++)
    {
      if(!items[i]->text(1).isEmpty())
	{
	  QDomElement element = domElementForItem.value(items[i]);
	  //QDomElement pElement = element.parentNode().toElement();
	  QString parentName = items[i]->parent()->text(0);
	  QString tName = domElementForItem.value(items[i]).tagName();
	  QDomElement pElement = configData->getRoot().firstChildElement(parentName);
	  QString paramValue = configData->getParam(pElement,tName);
	  if (paramValue!=items[i]->text(1))
	    {
	      items[i]->setText(1,paramValue);
	      QDomElement newElement = pElement.firstChildElement(tName);
	      domElementForItem.remove(items[i]);
	      domElementForItem.insert(items[i], newElement);
	      if (tName == QString("maxwavenumber"))
		{
		  QTreeWidgetItem *parentItem = items[i]->parent();
		  QDomElement element = domElementForItem[items[i]];
		  updateDataGaps(element, parentItem, paramValue);
		}      
	    }
	}
    }
  */
 read();
}

void ConfigTree::updateDataGaps(QDomElement element, 
				QTreeWidgetItem *parentItem,QString oldValue)
{
  QDomElement configSection = element.parentNode().toElement();
  if (element.text().toInt() > oldValue.toInt())
    {
      for (int i = oldValue.toInt()+1; i <= element.text().toInt(); i++)
	{
	  QString newTagName("maxdatagap_"+QString().setNum(i));
	  emit addDom(configSection, newTagName, QString().setNum(0));
	  QDomElement newElement = configSection.firstChildElement(newTagName);
	  
	  QTreeWidgetItem *newItem = createItem(newElement, parentItem);
	  QString paramName = newElement.tagName();
	  newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
	  newItem->setIcon(0, bookmarkIcon);
	  newItem->setText(0, paramName);
	  newItem->setText(1, newElement.text());
	}
    }
  else 
    {
      for (int i = element.text().toInt()+1; i <= oldValue.toInt(); i++)
	{
	  QString oldTagName("maxdatagap_"+QString().setNum(i));
	  emit removeDom(configSection, oldTagName);
	  QDomElement oldElement = configSection.firstChildElement(oldTagName);
	  int index = 0;
	  index = parentItem->indexOfChild(domElementForItem.key(oldElement));
	  if (index !=0)
	    parentItem->takeChild(index);
	  domElementForItem.remove(domElementForItem.key(oldElement));
	}
    }
  read();
}

void ConfigTree::catchLog(const Message& message)
{
  emit log(message);
}
