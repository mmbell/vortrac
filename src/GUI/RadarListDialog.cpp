/*
 * RadarListDialog.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 4/18/06
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "RadarListDialog.h"

#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDir>
#include <QCoreApplication>

RadarListDialog::RadarListDialog(QWidget *parent, Configuration *initialConfig)
  :QDialog(parent)
{
  /*
   * This dialog is used to update and edit the default radar list
   * which contains the latitude, longitude, and altitude many radars
   * that could potentially be of use in hurricane analysis. The data
   * assosiated with each radar was barrowed from the nex_tables.h file
   * from SoloII and are contained in the file vortrac_radarList.xml
   */

  this->setObjectName("Radar List Dialog");

  xmlFileName = QString("vortrac_radarList.xml");
 
  // radars is the Configuration class interface for working with xml files
  radars = initialConfig;

  QGroupBox *dialogBox = new QGroupBox(tr("Edit Radar Entry List"));
  editRadarButton = new QRadioButton(tr("Edit Existing Radar Entry"),
				     dialogBox);
  newRadarButton = new QRadioButton(tr("Create New Radar Entry"),
				    dialogBox);
  
   // Widgets and Layouts used for editing existing radar entries

  QFrame *editRadarFrame = new QFrame;

  QLabel *editRadarLabel = new QLabel(tr("Select a radar to edit"));
  readConfig();
  QVBoxLayout *editRadarLayout = new QVBoxLayout;
  editRadarLayout->addWidget(editRadarLabel);
  editRadarLayout->addWidget(editRadar);
    
  QLabel *editRadarNameLabel = new QLabel(tr("New radar name:"));
  editRadarName = new QLineEdit;
  QHBoxLayout *editRadarNameLayout = new QHBoxLayout;
  editRadarNameLayout->addWidget(editRadarNameLabel);
  editRadarNameLayout->addStretch();
  editRadarNameLayout->addWidget(editRadarName);
  
  QLabel *editRadarLatLabel = new QLabel(tr("Radar Latitude:"));
  editRadarLat = new QDoubleSpinBox;
  editRadarLat->setDecimals(3);
  editRadarLat->setRange(-999,999);
  QHBoxLayout *editRadarLatLayout = new QHBoxLayout;
  editRadarLatLayout->addWidget(editRadarLatLabel);
  editRadarLatLayout->addStretch();
  editRadarLatLayout->addWidget(editRadarLat);
    
  QLabel *editRadarLonLabel = new QLabel(tr("Radar Longitude:"));
  editRadarLon = new QDoubleSpinBox;
  editRadarLon->setDecimals(3);
  editRadarLon->setRange(-999,999);
  QHBoxLayout *editRadarLonLayout = new QHBoxLayout;
  editRadarLonLayout->addWidget(editRadarLonLabel);
  editRadarLonLayout->addStretch();
  editRadarLonLayout->addWidget(editRadarLon);

  QLabel *editRadarAltLabel = new QLabel(tr("Radar Altitude in meters:"));
  editRadarAlt = new QDoubleSpinBox;
  editRadarAlt->setDecimals(2);
  editRadarAlt->setRange(0,999);
  QHBoxLayout *editRadarAltLayout = new QHBoxLayout;
  editRadarAltLayout->addWidget(editRadarAltLabel);
  editRadarAltLayout->addStretch();
  editRadarAltLayout->addWidget(editRadarAlt);
  
  QPushButton *saveEdit = new QPushButton(tr("Save"));
  QHBoxLayout *saveEditLayout = new QHBoxLayout;
  saveEditLayout->addStretch();
  saveEditLayout->addWidget(saveEdit);
  
  QVBoxLayout *editLayout = new QVBoxLayout;
  editLayout->addLayout(editRadarLayout);
  editLayout->addLayout(editRadarNameLayout);
  editLayout->addLayout(editRadarLatLayout);
  editLayout->addLayout(editRadarLonLayout);
  editLayout->addLayout(editRadarAltLayout);
  editLayout->addLayout(saveEditLayout);
  editRadarFrame->setLayout(editLayout);
  editRadarFrame->hide();
  
  connect(editRadarButton, SIGNAL(toggled(bool)),
	  editRadarFrame, SLOT(setVisible(bool)));

  // Widgets and Layouts used for creating new radar entries

  QFrame *newRadarFrame = new QFrame;

  QLabel *newRadarNameLabel = new QLabel(tr("New radar name:"));
  newRadarName = new QLineEdit;
  QHBoxLayout *newRadarNameLayout = new QHBoxLayout;
  newRadarNameLayout->addWidget(newRadarNameLabel);
  newRadarNameLayout->addStretch();
  newRadarNameLayout->addWidget(newRadarName);

  QLabel *newRadarLocationLabel = new QLabel(tr("Location of new radar:"));
  newRadarLocation = new QLineEdit;
  QHBoxLayout *newRadarLocationLayout = new QHBoxLayout;
  newRadarLocationLayout->addWidget(newRadarLocationLabel);
  newRadarLocationLayout->addStretch();
  newRadarLocationLayout->addWidget(newRadarLocation);
  
  QLabel *newRadarLatLabel = new QLabel(tr("Radar Latitude:"));
  newRadarLat = new QDoubleSpinBox;
  newRadarLat->setDecimals(3);
  newRadarLat->setRange(-999,999);
  QHBoxLayout *newRadarLatLayout = new QHBoxLayout;
  newRadarLatLayout->addWidget(newRadarLatLabel);
  newRadarLatLayout->addStretch();
  newRadarLatLayout->addWidget(newRadarLat);
  
  QLabel *newRadarLonLabel = new QLabel(tr("Radar Longitude:"));
  newRadarLon = new QDoubleSpinBox;
  newRadarLon->setDecimals(3);
  newRadarLon->setRange(-999,999);
  QHBoxLayout *newRadarLonLayout = new QHBoxLayout;
  newRadarLonLayout->addWidget(newRadarLonLabel);
  newRadarLonLayout->addStretch();
  newRadarLonLayout->addWidget(newRadarLon);
  
  QLabel *newRadarAltLabel = new QLabel(tr("Radar Altitude in meters:"));
  newRadarAlt = new QDoubleSpinBox;
  newRadarAlt->setDecimals(2);
  newRadarAlt->setRange(0,999);
  QHBoxLayout *newRadarAltLayout = new QHBoxLayout;
  newRadarAltLayout->addWidget(newRadarAltLabel);
  newRadarAltLayout->addStretch();
  newRadarAltLayout->addWidget(newRadarAlt);
  
  QPushButton *saveNew = new QPushButton(tr("Save"));
  QHBoxLayout *saveNewLayout = new QHBoxLayout;
  saveNewLayout->addStretch();
  saveNewLayout->addWidget(saveNew);
  
  QVBoxLayout *newLayout = new QVBoxLayout;
  newLayout->addLayout(newRadarNameLayout);
  newLayout->addLayout(newRadarLocationLayout);
  newLayout->addLayout(newRadarLatLayout);
  newLayout->addLayout(newRadarLonLayout);
  newLayout->addLayout(newRadarAltLayout);
  newLayout->addLayout(saveNewLayout);
  newRadarFrame->setLayout(newLayout);
  
  connect(newRadarButton, SIGNAL(toggled(bool)),
	  newRadarFrame, SLOT(setVisible(bool)));
  
  // Main Layout

  QPushButton *done = new QPushButton(tr("Done"));
  done->setDefault(true);
  connect(done, SIGNAL(pressed()), this, SLOT(prepareToClose()));
  QHBoxLayout *buttons = new QHBoxLayout;
  buttons->addStretch(1);
  buttons->addWidget(done);

  QVBoxLayout *main = new QVBoxLayout;
  main->addWidget(editRadarButton);
  main->addWidget(editRadarFrame);
  main->addWidget(newRadarButton);
  main->addWidget(newRadarFrame);
  dialogBox->setLayout(main);
  
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(dialogBox);
  mainLayout->addStretch();
  mainLayout->addLayout(buttons);
  this->setLayout(mainLayout);

  // Connections for saving new data
  connect(saveEdit, SIGNAL(clicked()), this, SLOT(savePressed()));
  connect(saveNew, SIGNAL(clicked()), this, SLOT(savePressed()));

  // Connections for updating the Configuration
  connect(this, SIGNAL(changeDom(const QDomElement&, const QString&, 
				 const QString&)),
	  radars, SLOT(setParam(const QDomElement&, const QString&, 
				const QString&)));
  connect(this, SIGNAL(addDom(const QDomElement&, const QString&,
			      const QString&)),
	  radars, SLOT(addDom(const QDomElement&, const QString&, 
			      const QString&)));
  connect(editRadar, SIGNAL(activated(const QString&)), 
	  this, SLOT(radarValues(const QString&)));
 
  newRadarButton->setChecked(true);
  
}

RadarListDialog::~RadarListDialog()
{
  radars = NULL;
  delete radars;
  delete editRadarButton;
  delete newRadarButton;
  delete editRadar;
  delete editRadarName;
  delete newRadarName;
  delete newRadarLocation;
  delete editRadarLat;
  delete editRadarLon;
  delete editRadarAlt;
  delete newRadarLat;
  delete newRadarLon;
  delete newRadarAlt;
}

void RadarListDialog::readConfig()
{
  /* 
   * Reads the list of radar in the xml file and populates the drop down
   * menu on the screen. This is the same list used to reach this dialog
   * so the "other radars" option is removed.
   */

  editRadar = new QComboBox;
  QDomNodeList radarList = 
    radars->getRoot().childNodes();
  for (int i = 0; i <= radarList.count()-1; i++) 
    {
      QDomNode curNode = radarList.item(i);
      editRadar->addItem(curNode.firstChildElement(QString("text")).text());
    }
  editRadar->setEditable(false);
  int otherIndex = editRadar->findText(QString("Other"),Qt::MatchStartsWith);
  editRadar->removeItem(otherIndex);

}

void RadarListDialog::savePressed()
{
  /* 
   * This slot is used whenever one of the save buttons on either frame is 
   * pressed, the data shown on the cooresponding frame will be saved to the
   * xml radar list.
   */

  if(editRadarButton->isChecked()) {
    QString name = editRadarName->text();
    QDomElement oldRadar = 
      radars->getRoot().firstChildElement(editRadar->currentText().left(4));
    emit changeDom(oldRadar, QString("longitude"), 
		   QString().setNum(editRadarLon->value()));
    emit changeDom(oldRadar, QString("latitude"), 
		   QString().setNum(editRadarLat->value()));
    emit changeDom(oldRadar, QString("altitude"), 
		   QString().setNum(editRadarAlt->value()));
    QString newText = editRadar->currentText().remove(0, 4).prepend(name);
    emit changeDom(oldRadar, QString("text"), newText);
    oldRadar.setTagName(name);
    emit newEntry(newText);
  }
  if(newRadarButton->isChecked()) {
    QString name = newRadarName->text().left(4);
    emit addDom(radars->getRoot(), name, QString());
    QDomElement newDom = radars->getRoot().firstChildElement(name);
    emit addDom(newDom, QString("text"),
		name+" in "+newRadarLocation->text());
    emit addDom(newDom, QString("longitude"), 
		QString().setNum(newRadarLon->value()));
    emit addDom(newDom, QString("latitude"), 
		QString().setNum(newRadarLat->value()));
    emit addDom(newDom, QString("altitude"), 
		QString().setNum(newRadarAlt->value()));
    emit newEntry(name);
  }

  // Attempts to save changes made to the configuration perminant in the 
  // xml radar list
  QString resources = QCoreApplication::applicationDirPath() + "/../Resources";
  QDir resourceDir = QDir(resources);
  if(!radars->write(resourceDir.filePath(xmlFileName)))
    emit log(Message("Error Saving XML Radar List"));

  // Repopulates drop down menus with new information
  readConfig();
}

void RadarListDialog::catchLog(const Message& message)
{
  emit log(message);
}

void RadarListDialog::radarValues(const QString& choice)
{
  /* 
   * This slot is used whenever a radar is selected from the drop down list.
   * The latitude, longitude and altitude of the radar are displayed in the
   * appropriate display widgets.
   */

  QDomElement allRadars = radars->getRoot();
  QDomElement radar = allRadars.firstChildElement(choice.left(4));
  editRadarName->clear();
  editRadarName->setText(choice.left(4));
  editRadarLat->setValue(radar.firstChildElement("latitude").text().toFloat());
  editRadarLon->setValue(radar.firstChildElement("longitude").text().toFloat());
  editRadarAlt->setValue(radar.firstChildElement("altitude").text().toFloat());
}

void RadarListDialog::prepareToClose()
{
  // Implement more here as needed.

  // If a radar is added to the list it should be selected on the drop 
  // down menu in the configuration dialog

  close();
}
