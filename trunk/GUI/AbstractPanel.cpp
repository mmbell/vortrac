/*
 * 
 * AbstractPanel.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/30/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "AbstractPanel.h"
#include "Message.h"
#include "RadarListDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QDialog>
#include <QLineEdit>
#include <QRadioButton>
#include <QFrame>

#include <QFileDialog>

AbstractPanel::AbstractPanel(QWidget *parent)
  :QWidget(parent)
{
  defaultDirectory = new QDir(QDir::currentPath());
  dir = new QLineEdit;
  dir->setText(defaultDirectory->currentPath());
  browse = new QPushButton("Browse..");
  connectBrowse();
}

AbstractPanel::~AbstractPanel()
{
  delete dir;
  delete browse;
  delete defaultDirectory;
  delete maxWaveNumBox;
  delete dataGap;
  delete dataGapLayout;
  //  delete dataGapBoxes;
  for(int i = 0; i < dataGapBoxes.count(); i++) {
    delete dataGapBoxes[i];
  }
  //  delete dataGapLabels;
  for(int i = 0; i < dataGapLabels.count(); i++) {
    delete dataGapLabels[i];
  }
  delete radarLatBox;
  delete radarLongBox;
  delete radarAltBox;
  
}

void AbstractPanel::updatePanel(QDomElement panelElement)
{
  // This virtual function is reimplemented in inherited classes
  // to read in parameter values from the Configuration
}

void AbstractPanel::connectBrowse()
{
  disconnect(browse, SIGNAL(clicked()), 0, 0);
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
}

void AbstractPanel::connectFileBrowse()
{
  disconnect(browse, SIGNAL(clicked()), 0, 0);
  connect(browse, SIGNAL(clicked()), this, SLOT(getFileName()));
}

void AbstractPanel::getDirectory()
{
  QString filePath = QFileDialog::getExistingDirectory(this,
					     QString(tr("Select Directory")),
						       dir->text());
  dir->clear();
  dir->insert(filePath);
}

void AbstractPanel::getFileName()
{
  QString openFile;
  openFile=QFileDialog::getOpenFileName(this,
					QString("Select File"),
					dir->text(), 
					QString("Configuration Files (*.xml)"));
  dir->clear();
  dir->insert(openFile);
}

bool AbstractPanel::updateConfig()
{
  return true;
  // This virtual function is reimplemented in inherited classes
  // to update the Configuration to any changes in parameters made
  // in the ConfigurationDialog
}

void AbstractPanel::setPanelChanged(bool hasChanged)
{
  panelChanged = hasChanged;
}

void AbstractPanel::setElement(QDomElement newElement)
{
  elem = newElement;
}

void AbstractPanel::valueChanged()
{
  panelChanged = true;
  // updates the internal state of the panel when modifications were
  // made to it's members
}

void AbstractPanel::valueChanged(const bool signal)
{
  panelChanged = true;
  //overloaded function provided to accommodate member signals
}

void AbstractPanel::valueChanged(const QString& text)
{
  panelChanged = true;
  // overloaded function provided to accommodate member signals
}

void AbstractPanel::valueChanged(const QDateTime& dateTime)
{
  panelChanged = true;
  // overloaded function provided to accommodate member signals
}

void AbstractPanel::createDataGaps()
{
  if(dataGapBoxes.isEmpty())
    {
      dataGap = new QGroupBox(tr("Maximum Data Gaps"));
      dataGapLayout = new QGridLayout(dataGap);
      int row = 0;
      int column = 0;
      for(int i = 0; i <= 10;i++)
	{
	  QLabel *dataGapLabel = new QLabel("Wave "+QString().setNum(i));
	  dataGapLabels.append(dataGapLabel);
	  QDoubleSpinBox *dataGapBox = new QDoubleSpinBox;
	  dataGapBox->setRange(0, 999);
	  dataGapBox->setDecimals(1);
	  dataGapBox->setMinimumSize(dataGapBox->sizeHint());
	  dataGapBoxes.append(dataGapBox);
	  connect(dataGapBox, SIGNAL(valueChanged(const QString&)), 
		  this, SLOT(valueChanged(const QString&)));
      
	  dataGapLayout->addWidget(dataGapLabels[i], row, column);
	  dataGapLayout->addWidget(dataGapBoxes[i], row, column+1);
	  if (column==2) {
	    row++;
	    column = 0; }
	  else 
	    column+=2;
	  
	}
      dataGap->setLayout(dataGapLayout);
    }

  int num = maxWaveNumBox->value();
  
  for (int i = 10; i >=0; i--) {
    dataGapLabels[i]->setVisible(i<=num);
    dataGapBoxes[i]->setVisible(i<=num);
  }

  dataGap->setMinimumSize(dataGapLayout->minimumSize());
}

void AbstractPanel::createDataGaps(const QString& value)
{
  // Overloaded function
  createDataGaps();
}

void AbstractPanel::catchLog(const Message& message)
{
  emit log(message);
}

void AbstractPanel::checkForAnalytic(const QString& format)
{
  if(format ==  QString("Analytic Model"))
    connectFileBrowse();

}

void AbstractPanel::radarChanged(const QString& text)
{

  QDomElement allRadars = radars->getRoot();
  int initialCount = allRadars.childNodes().count();
  QString getEditPanel = allRadars.firstChildElement("OTHER").firstChildElement("text").text();
  if(text == getEditPanel){
    // If the other option is selected a panel appears for editing and adding
    // new radars to the existing radar list
    
    RadarListDialog *editRadar = new RadarListDialog(this, radars);
    
    updatePanel(elem);

    QDomNodeList radarList = 
      radars->getRoot().childNodes();
    radarName->clear();

    for (int i = 0; i <= radarList.count()-1; i++) 
      {
	QDomNode curNode = radarList.item(i);
	radarName->addItem(curNode.firstChildElement(QString("text")).text());
      }
    
    if(radarList.count() > initialCount)
      radarName->setCurrentIndex(radarList.count()-1);
    if(radarList.count() == initialCount)
      radarName->setCurrentIndex(0);
    
  }
  else {
    QDomElement radar = allRadars.firstChildElement(text.left(4));
    radarLatBox->setValue(radar.firstChildElement("latitude").text().toFloat());
    radarLongBox->setValue(radar.firstChildElement("longitude").text().toFloat());
    radarAltBox->setValue(radar.firstChildElement("altitude").text().toFloat());
  }
}

void AbstractPanel::setDefaultDirectory(QDir* newDir)
{
  if(!newDir->exists()) {
    newDir->mkpath(newDir->path());
  }
  if(!newDir->isAbsolute())
    newDir->makeAbsolute();
  
  defaultDirectory = newDir;
   
}
