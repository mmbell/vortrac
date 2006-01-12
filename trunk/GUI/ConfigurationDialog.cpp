/*
 * ConfigurationDialog.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/18/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "ConfigurationDialog.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QStackedWidget>
#include <QGridLayout>
#include <QPushButton>


ConfigurationDialog::ConfigurationDialog(Configuration *initialConfig)
  :configData(initialConfig)
{
  setWindowTitle(tr("VORTRAC CONFIGURATION"));
  panels = new QStackedWidget(this);
  populatePanels();
  makePanelForString();

  selection = new QListWidget(this);
  selection->setViewMode(QListView::IconMode);
  selection->setMovement(QListView::Static);
  selection->setMaximumWidth(210);
  //selection->setFlow(QListView::TopToBottom);
  populateSelection();
  connect(selection, 
  	  SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), 
  	  this, SLOT(switchPanel(QListWidgetItem *, QListWidgetItem *)));
  selection->setCurrentRow(0);

  QPushButton *apply = new QPushButton(tr("Apply"));
  QPushButton *cancel = new QPushButton(tr("Cancel"));
  connect(apply, SIGNAL(clicked()), this, SLOT(applyChanges()));
  connect(cancel, SIGNAL(clicked()), this, SLOT(close()));
  QHBoxLayout *buttons = new QHBoxLayout;
  buttons->addStretch(1);
  buttons->addWidget(apply);
  buttons->addWidget(cancel);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(panels);
  layout->addLayout(buttons);
  
  QHBoxLayout *main = new QHBoxLayout;
  main->addWidget(selection);
  main->addLayout(layout);
  setLayout(main);

}

void ConfigurationDialog::switchPanel(QListWidgetItem *current, QListWidgetItem *previous)
  //transitions between the panels in the QStacked Widget
{
  if(!current)
    current = previous;
  panels->setCurrentIndex(selection->row(current));
}

bool ConfigurationDialog::read()
{
  // Reads values from the Configuration
  if(!readConfig()) {
    emit log(Message("Configuration Dialog can't read the the Configuration"));
    return false; }

  return true;
}

bool ConfigurationDialog::readConfig()
{
  // This function iterates through all nodes of the Configuration
  // and assigns them to the correct panel.
  // Each panel then assigns all of its members the proper values

  QDomNodeList nodeList = configData->getRoot().childNodes();
  for (int i = 0; i <= nodeList.count()-1; i++) 
    {
      QDomNode currNode = nodeList.item(i);
      QDomElement child = currNode.toElement();
      panelForString[child.tagName()]->updatePanel(child);
    }
  return true;
  //find way to check fail!!
}

void ConfigurationDialog::applyChanges()
{
  // This function checks for panels whose members have been changed,
  // and saves the new values to their position in the configuration.

  QList<AbstractPanel*> panels = panelForString.values();
  for(int i = 0; i < panels.count();i++)
    {
      QString nodeName = panelForString.key(panels.at(i));
      if(!panels.at(i)->updateConfig())
	return;
    }
  emit configChanged();
  close();
}

void ConfigurationDialog::makePanelForString()
{
  // Sets up a Hash that will be used to assign Nodes in Configuration to
  // specific panel classes.

  panelForString.clear();
  panelForString.insert("vortex", vortex);
  panelForString.insert("radar", radar);
  panelForString.insert("cappi", cappi);
  panelForString.insert("center", center);
  panelForString.insert("vtd", vtd);
  panelForString.insert("hvvp", hvvp);
  panelForString.insert("pressure", pressure);
  panelForString.insert("graphics", graphics);
  panelForString.insert("qc", qc);
}

void ConfigurationDialog::populateSelection()
{
  // Creates names and icons used for selecting editing panels

  QListWidgetItem *vortex = new QListWidgetItem(selection);
  vortex->setText(tr("Vortex Configuration"));
  makeSelectionItem(vortex);
  vortex->setIcon(QIcon("images/DropSondeImage"));
 
  QListWidgetItem *radar = new QListWidgetItem(selection);
  radar->setText(tr("Radar Configuration"));
  makeSelectionItem(radar);
  radar->setIcon(QIcon("images/DropSondeImage"));
  
  QListWidgetItem *cappi = new QListWidgetItem(selection);
  cappi->setText(tr("CAPPI Configuration"));
  makeSelectionItem(cappi);
  cappi->setIcon(QIcon("images/DropSondeImage"));
  
  QListWidgetItem *center = new QListWidgetItem(selection);
  center->setText(tr("Simplex Configuration"));
  makeSelectionItem(center);
  center->setIcon(QIcon("images/DropSondeImage"));
  
  QListWidgetItem *vtd = new QListWidgetItem(selection);
  vtd->setText(tr("VTD Configuration"));
  makeSelectionItem(vtd);
  vtd->setIcon(QIcon("images/DropSondeImage"));
  
  QListWidgetItem *hvvp = new QListWidgetItem(selection);
  hvvp->setText(tr("HVVP Configuration"));
  makeSelectionItem(hvvp);
  hvvp->setIcon(QIcon("images/DropSondeImage"));
  
  QListWidgetItem *pressure = new QListWidgetItem(selection);
  pressure->setText(tr("Pressure Configuration"));
  makeSelectionItem(pressure);
  pressure->setIcon(QIcon("images/DropSondeImage"));
  
  QListWidgetItem *graphics = new QListWidgetItem(selection);
  graphics->setText(tr("Graphics Configuration"));
  makeSelectionItem(graphics);
  graphics->setIcon(QIcon("images/DropSondeImage"));

  QListWidgetItem *qc = new QListWidgetItem(selection);
  qc->setText(tr("Quality Control Configuration"));
  makeSelectionItem(qc);
  qc->setIcon(QIcon("images/DropSondeImage"));

}

void ConfigurationDialog::populatePanels()
{
  // Creates all the panels within the QStackedWidget

  vortex = new VortexPanel;
  panels->addWidget(vortex);
  connectPanel(vortex);
  radar = new RadarPanel;
  panels->addWidget(radar);
  connectPanel(radar);
  cappi = new CappiPanel;
  panels->addWidget(cappi);
  connectPanel(cappi);
  center = new CenterPanel;
  panels->addWidget(center);
  connectPanel(center);
  vtd = new VTDPanel;
  panels->addWidget(vtd);
  connectPanel(vtd);
  hvvp = new HVVPPanel;
  panels->addWidget(hvvp);
  connectPanel(hvvp);
  pressure = new PressurePanel;
  panels->addWidget(pressure);
  connectPanel(pressure);
  graphics = new GraphicsPanel;
  panels->addWidget(graphics);
  connectPanel(graphics);
  connect(graphics, SIGNAL(stateChange(const QString&, const bool)),
	  this, SLOT(stateChanged(const QString&, const bool)));
  connect(graphics, SIGNAL(changeDom(const QDomElement&, const QString&,
				     const QString&)),
	  this, SLOT(graphicsParameter(const QDomElement&, const QString&,
				       const QString&)));
  qc = new QCPanel;
  panels->addWidget(qc);
  connectPanel(qc);
}

void ConfigurationDialog::makeSelectionItem(QListWidgetItem * item)
{
  // Adds additional functionality to selection items
  // These specifications apply to each selection item

  // item->setIcon(QIcon("DropSondeImage"));
  item->setTextAlignment(Qt::AlignCenter);
  item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void ConfigurationDialog::checkConfig()
{
  // Checks to see if any modifications were made to the Configuration File

  if (configData->checkModified())
    readConfig();
}

void ConfigurationDialog::connectPanel(AbstractPanel* currPanel)
{
  // Makes connections that are similar for each panel

  connect(currPanel, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));

  connect(currPanel, SIGNAL(changeDom(const QDomElement&, 
				      const QString&, const QString&)), 
	  configData, SLOT(setParam(const QDomElement&, 
				    const QString&, const QString&)));
  connect(currPanel, SIGNAL(addDom(const QDomElement&, const QString&, 
				   const QString&)), 
	  configData, SLOT(addDom(const QDomElement&, const QString&, 
				  const QString&)));
  connect(currPanel, SIGNAL(removeDom(const QDomElement&, const QString&)),
	  configData, SLOT(removeDom(const QDomElement&, const QString&)));
}

void ConfigurationDialog::stateChanged(const QString& name, const bool change)
{
  emit stateChange(name, change);
}

void ConfigurationDialog::graphicsParameter(const QDomElement& element,
					    const QString& name, 
					    const QString& parameter)
{
  float num = parameter.toFloat();
  emit changeGraphicsParameter(name,num);
} 

void ConfigurationDialog::catchLog(const Message& message)
{
  emit log(message);
}
