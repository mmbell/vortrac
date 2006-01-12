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
#include <QGridLayout>
#include <QLabel>


#include <QFileDialog>

AbstractPanel::AbstractPanel(QWidget *parent)
  :QWidget(parent)
{
  dir = new QLineEdit;
  browse = new QPushButton("Browse..");
  connectBrowse();
}

void AbstractPanel::updatePanel(QDomElement panelElement)
{
  // This virtual function is reimplemented in inherited classes
  // to read in parameter values from the Configuration
}

void AbstractPanel::connectBrowse()
{
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
}

void AbstractPanel::getDirectory()
{
  QString filePath = QFileDialog::getExistingDirectory(this);
  dir->clear();
  dir->insert(filePath);
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
  
  /*
    int numberOfColumns = 0;
    for (int j = 4; j > 0; j--) {
    if ((dataGapBoxes.count()%j == 0)&&(j>numberOfColumns)) {
    numberOfColumns = j;
    } 
    }
    if (numberOfColumns==0); {
    for(int k = 4; k > 0; k--) {
    if((dataGapBoxes.count()/k > 0)&&(dataGapBoxes.count()%k >= k/2.0)
    &&(k>numberOfColumns)) {
    numberOfColumns = k;} } } 
    int row = 0;
    int column = 0;
    for(int i = 0; i < dataGapBoxes.count(); i++)
    {
    
    dataGapLayout->addWidget(dataGapLabels[i],row, column);
    column++;
    dataGapLayout->addWidget(dataGapBoxes[i], row, column);
    column++;
    if (column >= 2*numberOfColumns) {
    row++; 
    column = 0; }
    } */
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
