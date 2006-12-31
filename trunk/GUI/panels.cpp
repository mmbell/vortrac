/*
 * panels.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/18/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "panels.h"
#include "Message.h"

#include <QtGui>
#include <QFileDialog>

VortexPanel::VortexPanel()
{
  QLabel *vortexNameLabel = new QLabel(tr("Vortex Name:"));
  vortexName = new QLineEdit();
  QHBoxLayout *name = new QHBoxLayout;
  name->addWidget(vortexNameLabel);
  name->addWidget(vortexName);
  
  QGroupBox *stormType = new QGroupBox(tr("Cyclone Magnitude"));
  
  QRadioButton *hurricane = new QRadioButton(tr("Hurricane"), stormType);
  QRadioButton *tStorm = new QRadioButton(tr("Tropical Storm"), stormType);
  QRadioButton *tDepression = new QRadioButton(tr("Tropical Depression"), 
					       stormType);
  QVBoxLayout *stormTypeLayout = new QVBoxLayout;
  stormTypeLayout->addWidget(hurricane);
  stormTypeLayout->addWidget(tStorm);
  stormTypeLayout->addWidget(tDepression);
  stormType->setLayout(stormTypeLayout);

  QLabel *latLabel = new QLabel(tr("Vortex Latitude:"));
  latBox = new QDoubleSpinBox();
  latBox->setRange(-999,999);
  latBox->setDecimals(2);
  QHBoxLayout *lat = new QHBoxLayout;
  lat->addWidget(latLabel);
  lat->addStretch();
  lat->addWidget(latBox);
      
  QLabel *longLabel = new QLabel(tr("Vortex Longitude"));
  longBox = new QDoubleSpinBox();
  longBox->setRange(-999, 999);
  longBox->setDecimals(2);
  QHBoxLayout *longitude = new QHBoxLayout;
  longitude->addWidget(longLabel);
  longitude->addStretch();
  longitude->addWidget(longBox);

  QLabel *directionLabel = new QLabel(tr("Direction of vortex movement (degrees cw from north)"));
  directionBox = new QDoubleSpinBox();
  directionBox->setRange(0, 360);
  directionBox->setDecimals(2);
  QHBoxLayout *direction = new QHBoxLayout;
  direction->addWidget(directionLabel);
  direction->addStretch();
  direction->addWidget(directionBox);

  QLabel *speedLabel = new QLabel(tr("Speed of vortex movement (m/s)"));
  speedBox = new QDoubleSpinBox();
  speedBox->setRange(0,100);
  speedBox->setDecimals(2);
  QHBoxLayout *speed = new QHBoxLayout;
  speed->addWidget(speedLabel);
  speed->addStretch();
  speed->addWidget(speedBox);

  QLabel *workingDirLabel = new QLabel(tr("Working Directory"));
  // dir = new QLineEdit();
  //defaultDirectory = new QDir(QDir::currentPath());
  //browse = new QPushButton("Browse..");
  dir->setText(defaultDirectory->path());
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *dirLayout = new QGridLayout();
  dirLayout->addWidget(workingDirLabel, 0, 0);
  dirLayout->addWidget(dir, 1, 0, 1, 3);
  dirLayout->addWidget(browse, 1, 3);
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addLayout(name);
  layout->addWidget(stormType);
  layout->addLayout(lat);
  layout->addLayout(longitude);
  layout->addLayout(direction);
  layout->addLayout(speed);
  layout->addLayout(dirLayout);
  layout->addStretch(1);
  setLayout(layout);
  
  connect(vortexName, SIGNAL(textChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(latBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(longBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(directionBox, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(speedBox, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(dir, SIGNAL(textChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));

  setPanelChanged(false);
}

VortexPanel::~VortexPanel()
 {
  delete vortexName;
  delete latBox;
  delete longBox;
  delete directionBox;
  delete speedBox;
}
    
void VortexPanel::updatePanel(const QDomElement panelElement)
{
  // Sets the location of the panel's information in the Configuration,
  // iterates through all elements within this section of the Configuration,
  // and writes the values of these parameters to the corresponding member
  // widget.

  setElement(panelElement);
  QDomElement child = panelElement.firstChildElement();
  while (!child.isNull()) 
    {
      QString name = child.tagName();
      QString parameter = child.text();
      if(name == "name") {
	vortexName->clear();
	vortexName->insert(parameter); }
      if (name == "lat") {
	latBox->setValue(parameter.toDouble()); }
      if (name == "lon") {
	longBox->setValue(parameter.toDouble()); }
      if(name == "direction"){
	directionBox->setValue(parameter.toDouble()); }
      if(name == "speed") {
	speedBox->setValue(parameter.toDouble());}
      if(name == "dir")  {
	if(parameter!=QString("default")) {
	  dir->clear();
	  dir->insert(parameter); 
	  emit workingDirectoryChanged();}
	else {
	  setPanelChanged(true);}}
    
      child = child.nextSiblingElement();
    }
  setPanelChanged(false);
}

bool VortexPanel::updateConfig()
{
  // If any of the Panel's members have been changed these values will be
  // writen to the corresponding location within the Configuration

  QDomElement element = getPanelElement();
  if (checkPanelChanged())
    {
      if(getFromElement("name")!=vortexName->text()) {
	emit changeDom(element, "name", vortexName->text());
      }
      if(getFromElement("lat").toDouble()!=latBox->value()) {
	emit changeDom(element, QString("lat"), 
		       QString().setNum(latBox->value()));
      }
      if(getFromElement("lon").toDouble()!=longBox->value()) {
	emit changeDom(element, QString("lon"), 
		       QString().setNum(longBox->value()));
      }
      if(getFromElement("direction").toDouble()!=directionBox->value()) {
	emit changeDom(element, QString("direction"), 
		       QString().setNum(directionBox->value()));
      }
      if(getFromElement("speed").toDouble()!=speedBox->value()) {
	emit changeDom(element, QString("speed"), 
		       QString().setNum(speedBox->value()));
      }
      if(getFromElement("dir")!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
	emit workingDirectoryChanged();
      }
      
    }
  setPanelChanged(false);
  return true;
}

RadarPanel::RadarPanel()
{
  QLabel *radarNameLabel = new QLabel(tr("Radar Name:"));
  radarName = new QComboBox();
  radars = new Configuration(this,QDir::current().filePath(QString("vortrac_radarList.xml")));
  connect(radars, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
  QDomNodeList radarList = 
    radars->getRoot().childNodes();
  for (int i = 0; i <= radarList.count()-1; i++) 
    {
      QDomNode curNode = radarList.item(i);
      radarName->addItem(curNode.firstChildElement(QString("text")).text());
    }
  radarName->setEditable(false);
  
  QHBoxLayout *name = new QHBoxLayout;
  name->addWidget(radarNameLabel);
  name->addWidget(radarName);

  QLabel *latLabel = new QLabel(tr("Radar Latitude:"));
  //radarLatBox = new QDoubleSpinBox;
  radarLatBox->setRange(-999,999);
  radarLatBox->setDecimals(3);
  QHBoxLayout *lat = new QHBoxLayout;
  lat->addWidget(latLabel);
  lat->addWidget(radarLatBox);

  QLabel *longLabel = new QLabel(tr("Radar Longitude:"));
  //radarLongBox = new QDoubleSpinBox();
  radarLongBox->setDecimals(3);
  radarLongBox->setRange(-999, 999);
  QHBoxLayout *longitude = new QHBoxLayout;
  longitude->addWidget(longLabel);
  longitude->addWidget(radarLongBox);

  QLabel *altLabel = new QLabel(tr("Radar Altitude (meters):"));
  // radarAltBox = new QDoubleSpinBox();
  radarAltBox->setDecimals(3);
  radarAltBox->setRange(-999, 999);
  QHBoxLayout *altitude = new QHBoxLayout;
  altitude->addWidget(altLabel);
  altitude->addWidget(radarAltBox);

  QLabel *radarDirLabel = new QLabel(tr("Radar Data Directory"));
  // dir = new QLineEdit();
  //defaultDirectory = new QDir(QDir::currentPath());
  //browse = new QPushButton("Browse..");
  dir->setText(defaultDirectory->path());
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *radarDirLayout = new QGridLayout();
  radarDirLayout->addWidget(radarDirLabel, 0, 0);
  radarDirLayout->addWidget(dir, 1, 0, 1, 3);
  radarDirLayout->addWidget(browse, 1, 3);

  QLabel *radarFormatLabel = new QLabel(tr("Data Format"));
  radarFormatOptions = new QHash<QString, QString>;
  radarFormatOptions->insert(QString("Select a Radar Data Format"),
			     QString(""));
  radarFormatOptions->insert(QString("NCDC Level II"), QString("NCDCLEVELII"));
  radarFormatOptions->insert(QString("LDM Level II"), QString("LDMLEVELII"));
  radarFormatOptions->insert(QString("Analytic Model"), QString("MODEL"));
  radarFormat = new QComboBox();
  QList<QString> options = radarFormatOptions->keys();
  for(int i = 0; i < options.count(); i++) 
    {
      radarFormat->addItem(options[i]);
    }
  radarFormat->setEditable(false);
  QHBoxLayout *radarFormatLayout = new QHBoxLayout;
  radarFormatLayout->addWidget(radarFormatLabel);
  radarFormatLayout->addWidget(radarFormat);
  // add more formats when found

  QLabel *start = new QLabel(tr("Start Date and Time"));
  startDateTime = new QDateTimeEdit();
  startDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
  QHBoxLayout *startLayout = new QHBoxLayout;
  startLayout->addWidget(start);
  startLayout->addWidget(startDateTime);

  QLabel *end = new QLabel(tr("End Date and Time"));
  endDateTime = new QDateTimeEdit();
  endDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
  QHBoxLayout *endLayout = new QHBoxLayout;
  endLayout->addWidget(end);
  endLayout->addWidget(endDateTime);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addLayout(name);
  mainLayout->addLayout(lat);
  mainLayout->addLayout(longitude);
  mainLayout->addLayout(altitude);
  mainLayout->addLayout(radarDirLayout);
  mainLayout->addLayout(radarFormatLayout);
  mainLayout->addLayout(startLayout);
  mainLayout->addLayout(endLayout);
  mainLayout->addStretch(1);
  setLayout(mainLayout);

  connect(radarName, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged()));
  connect(radarLatBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(radarLongBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(dir, SIGNAL(textChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(radarFormat, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(startDateTime, SIGNAL(dateTimeChanged(const QDateTime&)), 
	  this, SLOT(valueChanged(const QDateTime&)));
  connect(endDateTime, SIGNAL(dateTimeChanged(const QDateTime&)), 
	  this, SLOT(valueChanged(const QDateTime&)));
  connect(radarFormat, SIGNAL(activated(const QString&)),
	  this, SLOT(checkForAnalytic(const QString&)));

  connect(radarName, SIGNAL(activated(const QString&)),
	  this, SLOT(radarChanged(const QString&)));
  setPanelChanged(false);

}

RadarPanel::~RadarPanel()
{
  delete radarFormat;
  delete radarFormatOptions;
  delete startDateTime;
  delete endDateTime;
}

void RadarPanel::updatePanel(const QDomElement panelElement)
{
  // Sets the location of the panel's information in the Configuration
  // Iterates through all elements within this section of the Configuration
  // and writes the values of these parameters to the corresponding member
  // widget.

  setElement(panelElement);
  QDomElement child = panelElement.firstChildElement();
  while (!child.isNull()) {
    QString name = child.tagName();
    QString parameter = child.text();
    if(name == "name") {
      int index = radarName->findText(parameter, 
				      Qt::MatchStartsWith);
      if (index != -1)
	radarName->setCurrentIndex(index);
      else 
	radarName->setCurrentIndex(0);}
    if (name == "lat") {
      radarLatBox->setValue(parameter.toFloat()); }
    if (name == "lon") {
      radarLongBox->setValue(parameter.toFloat()); }
    if(name == "alt") {
      radarAltBox->setValue(parameter.toFloat()); }
    if (name == "dir") {
      if(parameter!=QString("default")) {
	dir->clear();
	dir->insert(parameter); 
	emit workingDirectoryChanged();}
      else
	setPanelChanged(true);}
    if (name == "format") {
      if(parameter == QString("MODEL"))
	connectFileBrowse(); 
      else 
	connectBrowse();
      int index = radarFormat->findText(radarFormatOptions->key(parameter), 
					Qt::MatchStartsWith);
      if (index != -1)
	radarFormat->setCurrentIndex(index);
    }
    if (name == "startdate") {
      startDateTime->setDate(QDate::fromString(parameter, "yyyy-MM-dd")); }
    if (name == "enddate") {
      endDateTime->setDate(QDate::fromString(parameter, "yyyy-MM-dd")); }
    if (name == "starttime") {
      startDateTime->setTime(QTime::fromString(parameter, "hh:mm:ss")); }
    if (name == "endtime") {
      endDateTime->setTime(QTime::fromString(parameter, "hh:mm:ss")); }
    child = child.nextSiblingElement();
  }
  setPanelChanged(false);
}

bool RadarPanel::updateConfig()
{
  // If any the Panel's members have been changed these values will be
  // writen to the corresponding location within the Configuration

  QDomElement element = getPanelElement();
  if (checkPanelChanged())
    {
      if(startDateTime->dateTime() >= endDateTime->dateTime()) {
	emit log(Message("Start Date and Time must occur before End Date and Time"));
	return false;
      }
      if(getFromElement("name")!= radarName->currentText().left(4)) {
	emit changeDom(element, "name", 
		       radarName->currentText().left(4));
      }
      if(getFromElement("lat").toDouble() !=radarLatBox->value()) {
	emit changeDom(element, QString("lat"), 
		       QString().setNum(radarLatBox->value()));
      }
      if(getFromElement("lon").toDouble() !=radarLongBox->value()) {
	emit changeDom(element, QString("lon"), 
		       QString().setNum(radarLongBox->value()));
      }
      if(getFromElement("alt").toFloat() !=radarAltBox->value()) {
	emit changeDom(element, QString("alt"),
		       QString().setNum(radarAltBox->value()));
      }
      if(getFromElement("dir")!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
	emit workingDirectoryChanged();
      }
      if(getFromElement("startdate")
	 !=startDateTime->date().toString("yyyy-MM-dd")) {
	emit changeDom(element, QString("startdate"), 
		       startDateTime->date().toString("yyyy-MM-dd"));
      }
      if(getFromElement("enddate")
	 !=endDateTime->date().toString("yyyy-MM-dd")) {
	emit changeDom(element, QString("enddate"), 
		       endDateTime->date().toString("yyyy-MM-dd"));
      }
      if(getFromElement("starttime")
	 !=startDateTime->time().toString("hh:mm:ss")) {
	emit changeDom(element, QString("starttime"), 
		       startDateTime->time().toString("hh:mm:ss"));
      }
      if(getFromElement("endtime")
	 !=endDateTime->time().toString("hh:mm:ss")) {
	emit changeDom(element, QString("endtime"), 
		       endDateTime->time().toString("hh:mm:ss"));
      }
      if(getFromElement("format")
	 !=radarFormatOptions->value(radarFormat->currentText())) {
	if (radarFormat->currentText()==QString("Analytic Model")) 
	  connectFileBrowse();
	else
	  connectBrowse();
	emit changeDom(element, QString("format"), 
		       radarFormatOptions->value(radarFormat->currentText()));
      }
    }
  setPanelChanged(false);
  return true;
}


CappiPanel::CappiPanel()
{
  QLabel *cappiDirLabel = new QLabel(tr("CAPPI Output Directory"));
  //dir = new QLineEdit;
  //defaultDirectory = new QDir(QDir::currentPath());
  if(!defaultDirectory->exists(defaultDirectory->filePath("cappi"))) {
    defaultDirectory->mkdir("cappi") ;
  }
  defaultDirectory->cd("cappi");
 
  dir->setText(defaultDirectory->path());
  //browse = new QPushButton(tr("Browse.."));
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *cappiDir = new QGridLayout;
  cappiDir->addWidget(cappiDirLabel, 0, 0);
  cappiDir->addWidget(dir, 1, 0, 1, 3);
  cappiDir->addWidget(browse, 1,3);

  QGroupBox *grid = new QGroupBox(tr("Griding Configurations"));

  QLabel *xdim = new QLabel(tr("Grid Length in X Direction (km)"));
  QLabel *ydim = new QLabel(tr("Grid Length in Y Direction (km)"));
  QLabel *zdim = new QLabel(tr("Grid Length in Z Direction (km)"));
  xDimBox = new QDoubleSpinBox;
  xDimBox->setDecimals(1);
  xDimBox->setRange(0,1000000);
  yDimBox = new QDoubleSpinBox;
  yDimBox->setDecimals(1);
  yDimBox->setRange(0,1000000);
  zDimBox = new QDoubleSpinBox;
  zDimBox->setDecimals(1);
  zDimBox->setRange(0,1000000);

  QLabel *xGrid = new QLabel(tr("X Grid Spacing (km)"));
  xGridBox = new QDoubleSpinBox;
  xGridBox->setDecimals(1);
  xGridBox->setRange(0,10000);

  QLabel *yGrid = new QLabel(tr("Y Grid Spacing (km)"));
  yGridBox = new QDoubleSpinBox;
  yGridBox->setDecimals(1);
  yGridBox->setRange(0,10000);

  QLabel *zGrid = new QLabel(tr("Z Grid Spacing (km)"));
  zGridBox = new QDoubleSpinBox;
  zGridBox->setDecimals(1);
  zGridBox->setRange(0,10000);

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(xdim, 0, 0);
  gridLayout->addWidget(ydim, 0, 1);
  gridLayout->addWidget(zdim, 0, 2);
  gridLayout->addWidget(xDimBox, 1, 0);
  gridLayout->addWidget(yDimBox, 1, 1);
  gridLayout->addWidget(zDimBox, 1, 2);
  gridLayout->addWidget(xGrid, 2, 0);
  gridLayout->addWidget(yGrid, 2, 1);
  gridLayout->addWidget(zGrid, 2, 2);
  gridLayout->addWidget(xGridBox, 3, 0);
  gridLayout->addWidget(yGridBox, 3, 1);
  gridLayout->addWidget(zGridBox, 3, 2);
  grid->setLayout(gridLayout);

  // These need to be changed to U & V parameters !!!
  QLabel *advUWindLabel = new QLabel(tr("Zonal Advection Wind (m/s)"));
  advUWindBox = new QDoubleSpinBox;
  advUWindBox->setDecimals(1);
  advUWindBox->setRange(0,50000);
  QLabel *advVWindLabel = new QLabel(tr("Meridional Advection Wind (m/s)"));
  advVWindBox = new QDoubleSpinBox;
  advVWindBox->setDecimals(1);
  advVWindBox->setRange(0,359.99);

  QHBoxLayout *adv = new QHBoxLayout;
  adv->addWidget(advUWindLabel);
  adv->addWidget(advUWindBox);
  adv->addWidget(advVWindLabel);
  adv->addWidget(advVWindBox);

  QLabel *interpolation = new QLabel(tr("Interpolation"));
  interpolationMethod = new QHash<QString, QString>;
  interpolationMethod->insert(QString("Cressman Interpolation"),
			      QString("cressman"));
  interpolationMethod->insert(QString("Barnes Interpolation"),
			      QString("barnes"));
  interpolationMethod->insert(QString("Select Interpolation Method"), 
			      QString(""));
  // add some more of these interpolation method options as nessecary

  intBox = new QComboBox;
  QList<QString> options = interpolationMethod->keys();
  for(int i = 0; i < options.count(); i++)
    {
      intBox->addItem(options[i]);
    }
  QHBoxLayout *interpolationLayout = new QHBoxLayout;
  interpolationLayout->addWidget(interpolation);
  interpolationLayout->addWidget(intBox);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addLayout(cappiDir);
  layout->addWidget(grid);
  layout->addLayout(adv);
  layout->addLayout(interpolationLayout);
  layout->addStretch(1);
  setLayout(layout);

  connect(dir, SIGNAL(textChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(xDimBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(yDimBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(zDimBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(xGridBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(yGridBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(zGridBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(advUWindBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(advVWindBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(intBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  // These connections allow individual widgets to notify the panel when 
  // a parameter has changed.

  setPanelChanged(false);
}

CappiPanel::~CappiPanel()
{
  delete xDimBox;
  delete yDimBox;
  delete zDimBox;
  delete xGridBox;
  delete yGridBox;
  delete zGridBox;
  delete advUWindBox;
  delete advVWindBox;
  delete intBox;
  delete interpolationMethod;
}

void CappiPanel::updatePanel(const QDomElement panelElement)
{
  // Sets the location of the panel's information in the Configuration
  // Iterates through all elements within this section of the Configuration
  // and writes the values of these parameters to the corresponding member
  // widget.

  setElement(panelElement);
  QDomElement child = panelElement.firstChildElement();
  while (!child.isNull()) {
    QString name = child.tagName();
    QString parameter = child.text();
    if (name == "dir") {
      if(parameter!=QString("default")) {
	dir->clear();
	dir->insert(parameter); 
	emit workingDirectoryChanged();}
      else {
	setPanelChanged(true);}}
    if (name == "xdim") {
      xDimBox->setValue(parameter.toDouble()); }
    if (name == "ydim") {
      yDimBox->setValue(parameter.toDouble()); }
    if (name == "zdim") {
      zDimBox->setValue(parameter.toDouble()); }
    if (name == "xgridsp") {
      xGridBox->setValue(parameter.toDouble()); }
    if (name == "ygridsp") {
      yGridBox->setValue(parameter.toDouble()); }
    if (name == "zgridsp") {
      zGridBox->setValue(parameter.toDouble()); }
    if (name == "adv_u") {
      advUWindBox->setValue(parameter.toDouble()); }
    if( name == "adv_v") {
      advVWindBox->setValue(parameter.toDouble()); }
    if (name == "interpolation") {
      int index = intBox->findText(interpolationMethod->key(parameter),
				   Qt::MatchStartsWith);
      if (index != -1)
	intBox->setCurrentIndex(index);
    }
    child = child.nextSiblingElement();
  }
  setPanelChanged(false);
}

bool CappiPanel::updateConfig()
{
  // If any the Panel's members have been changed these values will be
  // writen to the corresponding location within the Configuration

  QDomElement element = getPanelElement();
  if (checkPanelChanged())
    {
      if(getFromElement("dir")!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
	emit workingDirectoryChanged();
      }
      if(getFromElement("xdim").toDouble() !=xDimBox->value()) {
	emit changeDom(element, QString("xdim"), 
		       QString().setNum(xDimBox->value()));
      }
      if(getFromElement("ydim").toDouble() !=yDimBox->value()) {
	emit changeDom(element, QString("ydim"), 
		       QString().setNum(yDimBox->value()));
      }
      if(getFromElement("zdim").toDouble() !=zDimBox->value()) {
    emit changeDom(element, QString("zdim"), 
		   QString().setNum(zDimBox->value()));
      }
      if(getFromElement("xgridsp").toDouble() !=xGridBox->value()) {
	emit changeDom(element, QString("xgridsp"), 
		       QString().setNum(xGridBox->value()));
      }
      if(getFromElement("ygridsp").toDouble() !=yGridBox->value()) {
	emit changeDom(element, QString("ygridsp"), 
		       QString().setNum(yGridBox->value()));
      }
      if(getFromElement("zgridsp").toDouble() !=zGridBox->value()) {
	emit changeDom(element, QString("zgridsp"), 
		       QString().setNum(zGridBox->value()));
      }
      if(getFromElement("adv_u").toDouble() !=advUWindBox->value()) {
	emit changeDom(element, QString("adv_u"), 
		       QString().setNum(advUWindBox->value()));
      }
      if(getFromElement("adv_v").toDouble() !=advVWindBox->value()) {
	emit changeDom(element, QString("adv_v"), 
		       QString().setNum(advVWindBox->value()));
      }
      if(getFromElement("interpolation") !=interpolationMethod->value(intBox->currentText())) 
	{
	  emit changeDom(element, QString("interpolation"),
			 interpolationMethod->value(intBox->currentText()));
	}
    }
  setPanelChanged(false);
  return true;
}

bool CappiPanel::setDefaultDirectory(QDir* newDir)
{
  if(!newDir->isAbsolute())
    newDir->makeAbsolute();
  QString subDirectory("cappi");
  if(newDir->exists(subDirectory))
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  if(newDir->mkdir(subDirectory)) {
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  }
  else {
    defaultDirectory = newDir;
    return false;
  }
  
}

CenterPanel::CenterPanel()
{
  QLabel *dirLabel = new QLabel(tr("Center Output Directory"));
  // dir = new QLineEdit();
  //defaultDirectory = new QDir(QDir::currentPath());
  if(!defaultDirectory->exists(defaultDirectory->filePath("center"))) {
    defaultDirectory->mkdir("center") ;
  }
  defaultDirectory->cd("center");
  //browse = new QPushButton(tr("Browse.."));
  dir->setText(defaultDirectory->path());
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *dirLayout = new QGridLayout;
  dirLayout->addWidget(dirLabel, 0, 0);
  dirLayout->addWidget(dir, 1, 0, 1, 3);
  dirLayout->addWidget(browse, 1,3);

  QLabel *geometry = new QLabel(tr("Geometry"));
  geometryOptions = new QHash<QString, QString>;
  geometryOptions->insert(QString("GBVTD"),
			      QString("GBVTD"));
  geometryOptions->insert(QString("Select Geometry"), 
			      QString(""));
  // Add additional option here

  geometryBox = new QComboBox;
  QList<QString> options = geometryOptions->keys();
  for(int i = 0; i < options.count(); i++)
    {
      geometryBox->addItem(options[i]);
    }

  QLabel *closure = new QLabel(tr("Closure"));
  closureOptions = new QHash<QString, QString>;
  closureOptions->insert(QString("Original"), 
			QString("original"));
  closureOptions->insert(QString("Select closure assumption"),
			 QString(""));
  // Add addtional options here

  closureBox = new QComboBox;
  options = closureOptions->keys();
  for(int i = 0; i < options.count(); i++)
    {
      closureBox->addItem(options[i]);
    }

  QLabel *reflectivity = new QLabel(tr("Reflectivity"));
  reflectivityOptions = new QHash<QString, QString>;
  reflectivityOptions->insert(QString("DZ"),
			      QString("DZ"));
  reflectivityOptions->insert(QString("Select reflectivity"),
			      QString(""));
  // Add additional options here
  
  refBox = new QComboBox;
  options = reflectivityOptions->keys();
  for(int i = 0; i < options.count(); i++)
    {
      refBox->addItem(options[i]);
    }

  QLabel *velocity = new QLabel(tr("Velocity"));
  velocityOptions = new QHash<QString, QString>;
  velocityOptions->insert(QString("VE"), QString("VE"));
  velocityOptions->insert(QString("Select Velocity"),
			  QString(""));
  // Add additional options here

  velBox = new QComboBox;
  options = velocityOptions->keys();
  for(int i = 0; i < options.count(); i++) 
    {
      velBox->addItem(options[i]);
    }

  QGroupBox *searchRegion = new QGroupBox(tr("Center Search Limitations"));
  QGridLayout *search = new QGridLayout;
  QLabel *bottomLevel = new QLabel(tr("Bottom Level (km)"));
  bLBox = new QSpinBox;
  QLabel *topLevel = new QLabel(tr("Top Level (km)"));
  tLBox = new QSpinBox;
  QLabel *innerRad = new QLabel(tr("Inner Radius (km)"));
  iRBox = new QSpinBox;
  QLabel *outerRad = new QLabel(tr("Outer Radius (km)"));
  oRBox = new QSpinBox;
  search->addWidget(bottomLevel, 0,0);
  search->addWidget(bLBox, 0, 1);
  search->addWidget(topLevel, 1, 0);
  search->addWidget(tLBox, 1, 1);
  search->addWidget(innerRad, 0, 2);
  search->addWidget(iRBox, 0, 3);
  search->addWidget(outerRad, 1, 2);
  search->addWidget(oRBox, 1, 3);
  searchRegion->setLayout(search);

  QGridLayout *optionsLayout = new QGridLayout;
  optionsLayout->addWidget(geometry, 0,0);
  optionsLayout->addWidget(geometryBox, 0, 1);
  optionsLayout->addWidget(closure, 1,0);
  optionsLayout->addWidget(closureBox,1,1);
  optionsLayout->addWidget(reflectivity, 0,3);
  optionsLayout->addWidget(refBox, 0,4);
  optionsLayout->addWidget(velocity, 1,3);
  optionsLayout->addWidget(velBox, 1, 4);

  dataGapBoxes = QList<QDoubleSpinBox*>();
  QLabel *maxWaveNum = new QLabel(tr("Maximum Wave Number"));
  // maxWaveNumBox = new QSpinBox;
  maxWaveNumBox->setRange(0, 10);
  maxWaveNumBox->setValue(0);
  QHBoxLayout *maxWaveLayout = new QHBoxLayout;
  maxWaveLayout->addWidget(maxWaveNum);
  maxWaveLayout->addWidget(maxWaveNumBox);
  createDataGaps();

  QLabel *searchCrit = new QLabel(tr("Center finding Criteria"));
  criteriaOptions = new QHash<QString,QString>;
  criteriaOptions->insert(QString("Maximum Tangential Velocity"),
			  QString("MAXVTO"));
  criteriaOptions->insert(QString(tr("Select Criteria")),
			  QString(""));
  // Add additional options here
	   
  critBox = new QComboBox;
  options = criteriaOptions->keys();
  for(int i = 0; i < options.count(); i++)
    {
      critBox->addItem(options[i]);
    }
  QHBoxLayout *criteriaLayout = new QHBoxLayout;
  criteriaLayout->addWidget(searchCrit);
  criteriaLayout->addWidget(critBox);

  QLabel *ringWidth = new QLabel(tr("Width of Search Rings (km)"));
  ringBox = new QDoubleSpinBox;
  ringBox->setDecimals(1);

  QLabel *influenceRadius = new QLabel(tr("Radius of Influence (km)"));
  influenceBox = new QDoubleSpinBox;
  influenceBox->setDecimals(1);

  QLabel *convergence = new QLabel(tr("Convergence Requirements"));
  convergenceBox = new QDoubleSpinBox;

  QLabel *maxIterations = new QLabel(tr("Maximum Iterations for Process"));
  iterations = new QSpinBox;

  QLabel *boxDiameter = new QLabel(tr("Width of Search Zone"));
  diameterBox = new QDoubleSpinBox;
  diameterBox->setDecimals(1);

  QLabel *numPoints = new QLabel(tr("Number of Center Points"));
  numPointsBox = new QSpinBox;

  QGridLayout *master = new QGridLayout;
  master->addWidget(ringWidth, 0, 0);
  master->addWidget(ringBox, 0, 1);
  master->addWidget(influenceRadius, 0, 2);
  master->addWidget(influenceBox, 0, 3);
  master->addWidget(convergence, 1,0);
  master->addWidget(convergenceBox, 1, 1);
  master->addWidget(maxIterations, 1, 2);
  master->addWidget(iterations, 1, 3);
  master->addWidget(boxDiameter, 2,0);
  master->addWidget(diameterBox, 2,1);
  master->addWidget(numPoints, 2,2);
  master->addWidget(numPointsBox, 2,3);

  QVBoxLayout *main = new QVBoxLayout;
  main->addLayout(dirLayout);
  main->addLayout(optionsLayout);
  main->addWidget(searchRegion);
  main->addLayout(maxWaveLayout);
  main->addWidget(dataGap);
  main->addLayout(criteriaLayout);
  main->addLayout(master);
  main->addStretch(1);
  setLayout(main);

  connect(dir, SIGNAL(textChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(geometryBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(closureBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(refBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(velBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(critBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(bLBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(tLBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(iRBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(oRBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(maxWaveNumBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(numPointsBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(iterations, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(ringBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(influenceBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(convergenceBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(diameterBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(maxWaveNumBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(createDataGaps(const QString&)));

  setPanelChanged(false);
}

CenterPanel::~CenterPanel()
{
  delete geometryBox;
  delete closureBox;
  delete refBox;
  delete velBox;
  delete critBox;
  delete geometryOptions;
  delete closureOptions;
  delete reflectivityOptions;
  delete velocityOptions;
  delete criteriaOptions;
  delete bLBox;
  delete tLBox;
  delete iRBox;
  delete oRBox;
  delete iterations;
  delete numPointsBox;
  delete ringBox;
  delete influenceBox;
  delete convergenceBox;
  delete diameterBox;
}

void CenterPanel::updatePanel(const QDomElement panelElement)
{
  // Sets the location of the panel's information in the Configuration
  // Iterates through all elements within this section of the Configuration
  // and writes the values of these parameters to the corresponding member
  // widget.

  setElement(panelElement);
  QDomElement child = panelElement.firstChildElement();
  while (!child.isNull()) {
    QString name = child.tagName();
    QString parameter = child.text();
    if (name == "dir") {
      if(parameter!=QString("default")) {
	dir->clear();
	dir->insert(parameter); 
	emit workingDirectoryChanged();}
      else 
	setPanelChanged(true);}
    if (name == "ringwidth") {
      ringBox->setValue(parameter.toDouble()); }
    if (name == "influenceradius") {
      influenceBox->setValue(parameter.toDouble()); }
    if (name == "convergence") {
      convergenceBox->setValue(parameter.toDouble()); }
    if (name == "boxdiameter") {
      diameterBox->setValue(parameter.toDouble()); }
    if (name == "bottomlevel") {
      bLBox->setValue(parameter.toInt()); }
    if (name == "toplevel") {
      tLBox->setValue(parameter.toInt()); }
    if (name == "innerradius") {
      iRBox->setValue(parameter.toInt()); }
    if (name == "outerradius") {
      oRBox->setValue(parameter.toInt()); }
    if (name == "maxwavenumber") {
      maxWaveNumBox->setValue(parameter.toInt()); 
      if(parameter.toInt() != dataGapBoxes.count()-1) {
	  createDataGaps();
	}
    }
    if(name == "maxdatagap") {
      int waveNum = child.attribute("wavenum").toInt();
      dataGapBoxes[waveNum]->setValue(parameter.toInt());
    }
    if (name == "maxiterations") {
      iterations->setValue(parameter.toInt()); }
    if (name == "numpoints") {
      numPointsBox->setValue(parameter.toInt()); }
    if (name == "geometry") {
      int index = geometryBox->findText(geometryOptions->key(parameter), 
					Qt::MatchStartsWith);
      if (index != -1)
	geometryBox->setCurrentIndex(index); }
    if (name == "closure") {
      int index = closureBox->findText(closureOptions->key(parameter), 
				       Qt::MatchStartsWith);
      if (index != -1)
	closureBox->setCurrentIndex(index); }
    if (name == "reflectivity") {
      int index = refBox->findText(reflectivityOptions->key(parameter), 
				   Qt::MatchStartsWith);
      if (index != -1)
	refBox->setCurrentIndex(index); }
    if (name == "velocity") {
      int index = velBox->findText(velocityOptions->key(parameter), 
				   Qt::MatchStartsWith);
      if (index != -1)
	velBox->setCurrentIndex(index); }
    if (name == "search") {
      int index = critBox->findText(criteriaOptions->key(parameter),
				    Qt::MatchStartsWith);
      if (index != -1)
	critBox->setCurrentIndex(index); }

    child = child.nextSiblingElement();
  }
 setPanelChanged(false);
}

bool CenterPanel::updateConfig()
{
  // If any the Panel's members have been changed these values will be
  // writen to the corresponding location within the Configuration

  QDomElement element = getPanelElement();
  if (checkPanelChanged())
    {
      if(getFromElement("dir")!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
	emit workingDirectoryChanged();
      }
      if(getFromElement("geometry")
	 !=geometryOptions->value(geometryBox->currentText())) {
	emit changeDom(element, QString("geometry"), 
		       geometryOptions->value(geometryBox->currentText()));
      }
      if(getFromElement("closure")
	!=closureOptions->value(closureBox->currentText()))
	{
	  emit changeDom(element, QString("closure"), 
			 closureOptions->value(closureBox->currentText()));
	}
      if(getFromElement("reflectivity")
	 !=reflectivityOptions->value(refBox->currentText())) {
	emit changeDom(element, QString("reflectivity"), 
		       reflectivityOptions->value(refBox->currentText()));
      }
      if(getFromElement("velocity")
	 !=velocityOptions->value(velBox->currentText())) {
	emit changeDom(element, QString("velocity"), 
		       velocityOptions->value(velBox->currentText()));
      }
      if(getFromElement("search")
	 !=criteriaOptions->value(critBox->currentText())) {
	emit changeDom(element, QString("search"), 
		       criteriaOptions->value(critBox->currentText()));
      }
      if(getFromElement("bottomlevel").toInt() !=bLBox->value()) {
	emit changeDom(element, QString("bottomlevel"), 
		       QString().setNum(bLBox->value()));
      }
      if(getFromElement("toplevel").toInt() !=tLBox->value()) {
	emit changeDom(element, QString("toplevel"), 
		       QString().setNum(tLBox->value()));
      }
      if(getFromElement("innerradius").toInt() !=iRBox->value()) {
	emit changeDom(element, QString("innerradius"), 
		       QString().setNum(iRBox->value()));
      }
      if(getFromElement("outerradius").toInt() !=oRBox->value()) {
	emit changeDom(element, QString("outerradius"), 
		       QString().setNum(oRBox->value()));
      }
      if(getFromElement("maxwavenumber").toInt() !=maxWaveNumBox->value()) {

	int box = maxWaveNumBox->value();
	int elem = getFromElement("maxwavenumber").toInt();

	emit changeDom(element, QString("maxwavenumber"), 
		       QString().setNum(maxWaveNumBox->value()));
	if(box < elem) {
	  QList<QDomElement> dataGapElements;
	  QDomElement dataGapChild = element.firstChildElement("maxdatagap");
	  while(!dataGapChild.isNull()&&
		(dataGapChild.tagName() == QString("maxdatagap"))) {
	    int waveNum = dataGapChild.attribute("wavenum").toInt();
	    if(waveNum > box) {
	      emit removeDom(element, QString("maxdatagap"), 
			     QString("wavenum"), QString().setNum(waveNum));
	    }
	    else {
	      emit changeDom(element, QString("maxdatagap"), 
			     QString().setNum(dataGapBoxes[waveNum]->value()),
			     QString("wavenum"), QString().setNum(waveNum));
	    }
	    dataGapChild = dataGapChild.nextSiblingElement("maxdatagap");
	  }
	}
	else {
	  for(int waveNum = elem+1; waveNum <= box; waveNum++) {
	    emit addDom(element, QString("maxdatagap"), 
			QString().setNum(dataGapBoxes[waveNum]->value()),
			QString("wavenum"), QString().setNum(waveNum));
	  }
	  for(int waveNum = 0; waveNum <= box; waveNum++) {
	    emit changeDom(element, QString("maxdatagap"),
			   QString().setNum(dataGapBoxes[waveNum]->value()),
			   QString("wavenum"), QString().setNum(waveNum));
	  }
	}
      }
      else{

      }
      if(getFromElement("ringwidth").toDouble() !=ringBox->value()) {
	emit changeDom(element, QString("ringwidth"), 
		       QString().setNum(ringBox->value()));
      }
      if(getFromElement("influenceradius").toDouble()
	 !=influenceBox->value()) {
	emit changeDom(element, QString("influenceradius"), 
		       QString().setNum(influenceBox->value()));
      }
      if(getFromElement("convergence").toDouble() !=convergenceBox->value()) {
	emit changeDom(element, QString("convergence"), 
		       QString().setNum(convergenceBox->value()));
      }
      if(getFromElement("maxiterations").toInt() !=iterations->value()) {
	emit changeDom(element, QString("maxiterations"), 
		       QString().setNum(iterations->value()));
      }
      if(getFromElement("boxdiameter").toDouble() !=diameterBox->value()) {
	emit changeDom(element, QString("boxdiameter"), 
		       QString().setNum(diameterBox->value()));
      }
      if(getFromElement("numpoints").toDouble() !=numPointsBox->value()) {
	emit changeDom(element, QString("numpoints"), 
		       QString().setNum(numPointsBox->value()));
      }
    }
  setPanelChanged(false);
  return true;
}

bool CenterPanel::setDefaultDirectory(QDir* newDir)
{
  QString subDirectory("center");
  if(!newDir->isAbsolute())
    newDir->makeAbsolute();
  if(newDir->exists(subDirectory))
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  if(newDir->mkdir(subDirectory)) {
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  }
  else {
    defaultDirectory = newDir;
    return false;
  }
  
}

ChooseCenterPanel::ChooseCenterPanel()
{
  QLabel *dirLabel = new QLabel(tr("VTD Output Directory"));
  // dir = new QLineEdit();
  //defaultDirectory = new QDir(QDir::currentPath());
  if(!defaultDirectory->exists(defaultDirectory->filePath("chooseCenter"))) {
    defaultDirectory->mkdir("chooseCenter") ;
  }
  defaultDirectory->cd("chooseCenter");
  //browse = new QPushButton(tr("Browse.."));
  dir->setText(defaultDirectory->path());
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *dirLayout = new QGridLayout;
  dirLayout->addWidget(dirLabel, 0, 0);
  dirLayout->addWidget(dir, 1, 0, 1, 3);
  dirLayout->addWidget(browse, 1,3);

  QLabel *start = new QLabel(tr("Start Date and Time"));
  startDateTime = new QDateTimeEdit();
  startDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
  QHBoxLayout *startLayout = new QHBoxLayout;
  startLayout->addWidget(start);
  startLayout->addWidget(startDateTime);

  QLabel *end = new QLabel(tr("End Date and Time"));
  endDateTime = new QDateTimeEdit();
  endDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
  QHBoxLayout *endLayout = new QHBoxLayout;
  endLayout->addWidget(end);
  endLayout->addWidget(endDateTime);

  QLabel* minVolumesLabel = new QLabel(tr("Number of Volumes Required to Begin Curve Fitting"));
  minVolumes = new QSpinBox();
  minVolumes->setRange(3,100);
  QHBoxLayout *minVolumesLayout = new QHBoxLayout();
  minVolumesLayout->addWidget(minVolumesLabel);
  minVolumesLayout->addStretch();
  minVolumesLayout->addWidget(minVolumes);

  QGroupBox *weightsMeanGroup = new QGroupBox(tr("Mean Weighting Scheme"));
  QGridLayout *weightsMeanLayout = new QGridLayout;
  QLabel *windWeightLabel = new QLabel(tr("Maximum Wind Score Weight"));
  windWeightBox = new QDoubleSpinBox;
  windWeightBox->setRange(0,1);
  windWeightBox->setDecimals(2);
  QLabel *stdDevWeightLabel = new QLabel(tr("Standard Deviation Score Weight"));
  stdDevWeightBox = new QDoubleSpinBox;
  stdDevWeightBox->setRange(0,1);
  stdDevWeightBox->setDecimals(2);
  QLabel *ptsWeightLabel = new QLabel(tr("Number of Converging Centers Score Weight"));
  ptsWeightBox = new QDoubleSpinBox;
  ptsWeightBox->setRange(0,1);
  ptsWeightBox->setDecimals(2);

  weightsMeanLayout->addWidget(windWeightLabel,0,0,1,1);
  weightsMeanLayout->addWidget(windWeightBox,0,1);
  weightsMeanLayout->addWidget(stdDevWeightLabel, 1,0,1,1);
  weightsMeanLayout->addWidget(stdDevWeightBox,1,1);
  weightsMeanLayout->addWidget(ptsWeightLabel, 2,0,1,1);
  weightsMeanLayout->addWidget(ptsWeightBox,2,1);
  weightsMeanGroup->setLayout(weightsMeanLayout);
  
  QGroupBox *weightsSingleGroup = new QGroupBox(tr("Center Weighting Scheme"));
  QGridLayout *weightsSingleLayout = new QGridLayout;
  QLabel *positionWeightLabel = new QLabel(tr("Distance Score Weight"));
  positionWeightBox = new QDoubleSpinBox;
  positionWeightBox->setRange(0,1);
  positionWeightBox->setDecimals(2);
  QLabel *rmwWeightLabel = new QLabel(tr("Radius of Maximum Wind Score Weight"));
  rmwWeightBox = new QDoubleSpinBox;
  rmwWeightBox->setRange(0,1);
  rmwWeightBox->setDecimals(2);
  QLabel *velWeightLabel = new QLabel(tr("Maximum Velocity Score Weight"));
  velWeightBox = new QDoubleSpinBox;
  velWeightBox->setRange(0,1);
  velWeightBox->setDecimals(2);

  weightsSingleLayout->addWidget(positionWeightLabel,0,0);
  weightsSingleLayout->addWidget(positionWeightBox,0,1);
  weightsSingleLayout->addWidget(rmwWeightLabel, 1,0);
  weightsSingleLayout->addWidget(rmwWeightBox,1,1);
  weightsSingleLayout->addWidget(velWeightLabel, 2,0);
  weightsSingleLayout->addWidget(velWeightBox,2,1);
  weightsSingleGroup->setLayout(weightsSingleLayout);

  QGroupBox *fTestGroup = new QGroupBox(tr("fTest Precision"));
  QHBoxLayout *fTestLayout = new QHBoxLayout;
  QLabel *f95Label = new QLabel(tr("95% Agreement"));
  fTest95Button = new QRadioButton(fTestGroup);
  QLabel *f99Label = new QLabel(tr("99% Agreement"));
  fTest99Button = new QRadioButton(fTestGroup);
  fTestLayout->addWidget(f95Label);
  fTestLayout->addWidget(fTest95Button);
  fTestLayout->addWidget(f99Label);
  fTestLayout->addWidget(fTest99Button);
  fTestGroup->setLayout(fTestLayout);

  QVBoxLayout *main = new QVBoxLayout;
  main->addLayout(dirLayout);
  main->addLayout(startLayout);
  main->addLayout(endLayout);
  main->addLayout(minVolumesLayout);
  main->addWidget(weightsMeanGroup);
  main->addWidget(weightsSingleGroup);
  main->addWidget(fTestGroup);
  main->addStretch(1);
  setLayout(main);

  connect(dir, SIGNAL(textChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(startDateTime, SIGNAL(dateTimeChanged(const QDateTime&)), 
	  this, SLOT(valueChanged(const QDateTime&)));
  connect(endDateTime, SIGNAL(dateTimeChanged(const QDateTime&)), 
	  this, SLOT(valueChanged(const QDateTime&)));
  connect(windWeightBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(stdDevWeightBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(ptsWeightBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(positionWeightBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(rmwWeightBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(velWeightBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(fTest95Button, SIGNAL(clicked(const bool)),
	  this, SLOT(valueChanged(const bool)));
  connect(fTest95Button, SIGNAL(clicked(const bool)),
	  this, SLOT(valueChanged(const bool)));
  connect(minVolumes, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));

  setPanelChanged(false);
}

ChooseCenterPanel::~ChooseCenterPanel()
{
  delete startDateTime;
  delete endDateTime;
  delete windWeightBox;
  delete stdDevWeightBox;
  delete ptsWeightBox;
  delete positionWeightBox;
  delete rmwWeightBox;
  delete velWeightBox;
  delete fTest95Button;
  delete fTest99Button;
  delete minVolumes;
}

void ChooseCenterPanel::updatePanel(const QDomElement panelElement)
{
  setElement(panelElement);
  QDomElement child = panelElement.firstChildElement();
  while (!child.isNull()) {
    QString name = child.tagName();
    QString parameter = child.text();
    if (name == "dir") {
      if(parameter!=QString("default")) {
	dir->clear();
	dir->insert(parameter); 
	emit workingDirectoryChanged();}
      else 
	setPanelChanged(true);}
    if (name == "startdate") {
      startDateTime->setDate(QDate::fromString(parameter, "yyyy-MM-dd")); }
    if (name == "enddate") {
      endDateTime->setDate(QDate::fromString(parameter, "yyyy-MM-dd")); }
    if (name == "starttime") {
      startDateTime->setTime(QTime::fromString(parameter, "hh:mm:ss")); }
    if (name == "endtime") {
      endDateTime->setTime(QTime::fromString(parameter, "hh:mm:ss")); }
    if (name == "wind_weight") {
      windWeightBox->setValue(parameter.toFloat()); }
    if (name == "stddev_weight") {
      stdDevWeightBox->setValue(parameter.toFloat()); }
    if (name == "pts_weight") {
      ptsWeightBox->setValue(parameter.toFloat()); }
    if (name == "position_weight") {
      positionWeightBox->setValue(parameter.toFloat()); }
    if (name == "rmw_weight") {
      rmwWeightBox->setValue(parameter.toFloat()); }
    if (name == "vt_weight") {
      velWeightBox->setValue(parameter.toFloat()); }
    if (name == "stats") {
      if(parameter.toInt() == 99)
	fTest99Button->setChecked(true);
      else
	fTest95Button->setChecked(true);
    }
    if (name == "min_volumes") {
      minVolumes->setValue(parameter.toInt()); 
    }
    child = child.nextSiblingElement();
  }
  setPanelChanged(false);
}
 
bool ChooseCenterPanel::updateConfig()
{
  QDomElement element = getPanelElement();
  if(checkPanelChanged())
    {
      if(startDateTime->dateTime() >= endDateTime->dateTime()) {
	emit log(Message("Start Date and Time must occur before End Date and Time"));
	return false;
      }
       if(getFromElement("startdate")
	 !=startDateTime->date().toString("yyyy-MM-dd")) {
	emit changeDom(element, QString("startdate"), 
		       startDateTime->date().toString("yyyy-MM-dd"));
      }
      if(getFromElement("enddate")
	 !=endDateTime->date().toString("yyyy-MM-dd")) {
	emit changeDom(element, QString("enddate"), 
		       endDateTime->date().toString("yyyy-MM-dd"));
      }
      if(getFromElement("starttime")
	 !=startDateTime->time().toString("hh:mm:ss")) {
	emit changeDom(element, QString("starttime"), 
		       startDateTime->time().toString("hh:mm:ss"));
      }
      if(getFromElement("endtime")
	 !=endDateTime->time().toString("hh:mm:ss")) {
	emit changeDom(element, QString("endtime"), 
		       endDateTime->time().toString("hh:mm:ss"));
      }
      if(getFromElement("dir")!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
	emit workingDirectoryChanged();
      }
      if(getFromElement("min_volumes").toInt() !=minVolumes->value()) {
	emit changeDom(element, QString("min_volumes"),
		       QString().setNum(minVolumes->value()));
      }
      if(getFromElement("wind_weight").toFloat() !=windWeightBox->value()) {
	emit changeDom(element, QString("wind_weight"),
		       QString().setNum(windWeightBox->value()));
      }
      if(getFromElement("stddev_weight").toFloat()!=stdDevWeightBox->value()) {
	emit changeDom(element, QString("stddev_weight"),
		       QString().setNum(stdDevWeightBox->value()));
      }
      if(getFromElement("pts_weight").toFloat() !=ptsWeightBox->value()) {
	emit changeDom(element, QString("pts_weight"),
		       QString().setNum(ptsWeightBox->value()));
      }
      if(getFromElement("position_weight").toFloat()
	 !=positionWeightBox->value()) {
	emit changeDom(element, QString("position_weight"),
		       QString().setNum(positionWeightBox->value()));
      }
      if(getFromElement("rmw_weight").toFloat() !=rmwWeightBox->value()) {
	emit changeDom(element, QString("rmw_weight"),
		       QString().setNum(rmwWeightBox->value()));
      }
      if(getFromElement("vt_weight").toFloat() !=velWeightBox->value()) {
	emit changeDom(element, QString("vt_weight"),
		       QString().setNum(velWeightBox->value()));
      }
      if(fTest99Button->isChecked()) {
	emit changeDom(element, QString("stats"), QString().setNum(99));
      }
      if(fTest95Button->isChecked()) {
	emit changeDom(element, QString("stats"), QString().setNum(95));
      }
    }
  setPanelChanged(false);
  return true;
}

bool ChooseCenterPanel::setDefaultDirectory(QDir* newDir)
{
  QString subDirectory("choosecenter");
  if(!newDir->isAbsolute())
    newDir->makeAbsolute();
  if(newDir->exists(subDirectory))
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  if(newDir->mkdir(subDirectory)) {
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  }
  else {
    defaultDirectory = newDir;
    return false;
  }
  
}

VTDPanel::VTDPanel()
{
  QLabel *dirLabel = new QLabel(tr("VTD Output Directory"));
  // dir = new QLineEdit();
  //defaultDirectory = new QDir(QDir::currentPath());
  if(!defaultDirectory->exists(defaultDirectory->filePath("vtd"))) {
    defaultDirectory->mkdir("vtd") ;
  }
  defaultDirectory->cd("vtd");
  //browse = new QPushButton(tr("Browse.."));
  dir->setText(defaultDirectory->path());
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *dirLayout = new QGridLayout;
  dirLayout->addWidget(dirLabel, 0, 0);
  dirLayout->addWidget(dir, 1, 0, 1, 3);
  dirLayout->addWidget(browse, 1,3);

  QLabel *geometry = new QLabel(tr("Geometry"));
  geometryOptions = new QHash<QString, QString>;
  geometryOptions->insert(QString("GBVTD"),
			  QString("GBVTD"));
  geometryOptions->insert(QString("Select Geometry"), 
			  QString(""));
  // Add additional option here
  
  geometryBox = new QComboBox;
  QList<QString> options = geometryOptions->keys();
  for(int i = 0; i < options.count(); i++)
    {
      geometryBox->addItem(options[i]);
    }
  
  QLabel *closure = new QLabel(tr("Closure"));
  closureOptions = new QHash<QString, QString>;
  closureOptions->insert(QString("Original"), 
			QString("original"));
  closureOptions->insert(QString("Select closure assumption"),
			 QString(""));
  // Add addtional options here
  
  closureBox = new QComboBox;
  options = closureOptions->keys();
  for(int i = 0; i < options.count(); i++)
    {
      closureBox->addItem(options[i]);
    }
  
  QLabel *reflectivity = new QLabel(tr("Reflectivity"));
  reflectivityOptions = new QHash<QString, QString>;
  reflectivityOptions->insert(QString("DZ"),
			      QString("DZ"));
  reflectivityOptions->insert(QString("Select reflectivity"),
			      QString(""));
  // Add additional options here
  
  refBox = new QComboBox;
  options = reflectivityOptions->keys();
  for(int i = 0; i < options.count(); i++)
    {
      refBox->addItem(options[i]);
    }
  
  QLabel *velocity = new QLabel(tr("Velocity"));
  velocityOptions = new QHash<QString, QString>;
  velocityOptions->insert(QString("VE"), QString("VE"));
  velocityOptions->insert(QString("Select Velocity"),
			  QString(""));
  // Add additional options here

  velBox = new QComboBox;
  options = velocityOptions->keys();
  for(int i = 0; i < options.count(); i++) 
    {
      velBox->addItem(options[i]);
    }  

  QGroupBox *searchRegion = new QGroupBox(tr("VTD Grid Region"));
  QGridLayout *search = new QGridLayout;
  QLabel *bottomLevel = new QLabel(tr("Bottom Level (km)"));
  bLBox = new QSpinBox;
  QLabel *topLevel = new QLabel(tr("Top Level (km)"));
  tLBox = new QSpinBox;
  QLabel *innerRad = new QLabel(tr("Inner Radius (km)"));
  iRBox = new QSpinBox;
  QLabel *outerRad = new QLabel(tr("Outer Radius (km)"));
  oRBox = new QSpinBox;
  search->addWidget(bottomLevel, 0,0);
  search->addWidget(bLBox, 0, 1);
  search->addWidget(topLevel, 1, 0);
  search->addWidget(tLBox, 1, 1);
  search->addWidget(innerRad, 0, 2);
  search->addWidget(iRBox, 0, 3);
  search->addWidget(outerRad, 1, 2);
  search->addWidget(oRBox, 1, 3);
  searchRegion->setLayout(search);

  QGridLayout *optionsLayout = new QGridLayout;
  optionsLayout->addWidget(geometry, 0,0);
  optionsLayout->addWidget(geometryBox, 0, 1);
  optionsLayout->addWidget(closure, 1,0);
  optionsLayout->addWidget(closureBox,1,1);
  optionsLayout->addWidget(reflectivity, 0,3);
  optionsLayout->addWidget(refBox, 0,4);
  optionsLayout->addWidget(velocity, 1,3);
  optionsLayout->addWidget(velBox, 1,4);

  dataGapBoxes = QList<QDoubleSpinBox*>();
  QLabel *maxWaveNum = new QLabel(tr("Maximum Wave Number"));
  // maxWaveNumBox = new QSpinBox;
  maxWaveNumBox->setRange(0, 10);
  maxWaveNumBox->setValue(0);
  QHBoxLayout *maxWaveLayout = new QHBoxLayout;
  maxWaveLayout->addWidget(maxWaveNum);
  maxWaveLayout->addWidget(maxWaveNumBox);
  createDataGaps();

  QLabel *ringWidth = new QLabel(tr("Width of Search Rings"));
  ringBox = new QDoubleSpinBox;
  ringBox->setDecimals(1);
  QHBoxLayout *ringLayout = new QHBoxLayout;
  ringLayout->addWidget(ringWidth);
  ringLayout->addWidget(ringBox);

  QVBoxLayout *main = new QVBoxLayout;
  main->addLayout(dirLayout);
  main->addLayout(optionsLayout);
  main->addWidget(searchRegion);
  main->addLayout(maxWaveLayout);
  main->addWidget(dataGap);
  main->addLayout(ringLayout);
  main->addStretch(1);
  setLayout(main);

  connect(dir, SIGNAL(textChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(geometryBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(closureBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(refBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(velBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(bLBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(tLBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(iRBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(oRBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(maxWaveNumBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(ringBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(maxWaveNumBox, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(createDataGaps(const QString&)));

  setPanelChanged(false);
}

VTDPanel::~VTDPanel()
{
  delete geometryBox;
  delete closureBox;
  delete refBox;
  delete velBox;
  delete geometryOptions;
  delete closureOptions;
  delete reflectivityOptions;
  delete velocityOptions;
  delete bLBox;
  delete tLBox;
  delete iRBox;
  delete oRBox;
  delete ringBox;
}

void VTDPanel::updatePanel(const QDomElement panelElement)
{
  // Sets the location of the panel's information in the Configuration
  // Iterates through all elements within this section of the Configuration
  // and writes the values of these parameters to the corresponding member
  // widget.

  setElement(panelElement);
  QDomElement child = panelElement.firstChildElement();
  while (!child.isNull()) {
    QString name = child.tagName();
    QString parameter = child.text();
    if (name == "dir") {
      if(parameter!=QString("default")) {
	dir->clear();
	dir->insert(parameter); 
	emit workingDirectoryChanged();}
      else 
	setPanelChanged(true);}
    if (name == "ringwidth") {
      ringBox->setValue(parameter.toDouble()); }
    if (name == "bottomlevel") {
      bLBox->setValue(parameter.toInt()); }
    if (name == "toplevel") {
      tLBox->setValue(parameter.toInt()); }
    if (name == "innerradius") {
      iRBox->setValue(parameter.toInt()); }
    if (name == "outerradius") {
      oRBox->setValue(parameter.toInt()); }
    if (name == "maxwavenumber") {
      maxWaveNumBox->setValue(parameter.toInt()); 
      if(parameter.toInt() != dataGapBoxes.count()-1) {
	createDataGaps();  } }
    if(name == "maxdatagap") {
      int waveNum = child.attribute("wavenum").toInt();
      dataGapBoxes[waveNum]->setValue(parameter.toInt()); }
    if (name == "geometry") {
      int index = geometryBox->findText(geometryOptions->key(parameter), 
					Qt::MatchStartsWith);
      if (index != -1)
	geometryBox->setCurrentIndex(index); }
    if (name == "closure") {
      int index = closureBox->findText(closureOptions->key(parameter), 
				       Qt::MatchStartsWith);
      if (index != -1)
	closureBox->setCurrentIndex(index); }
    if (name == "reflectivity") {
      int index = refBox->findText(reflectivityOptions->key(parameter), 
				   Qt::MatchStartsWith);
      if (index != -1)
	refBox->setCurrentIndex(index); }
    if (name == "velocity") {
      int index = velBox->findText(velocityOptions->key(parameter), 
				   Qt::MatchStartsWith);
      if (index != -1)
	velBox->setCurrentIndex(index); }

    child = child.nextSiblingElement();
  }
  setPanelChanged(false);
}

bool VTDPanel::updateConfig()
{
  // If any the Panel's members have been changed these values will be
  // writen to the corresponding location within the Configuration

  QDomElement element = getPanelElement();
  if (checkPanelChanged())
    {
      if(getFromElement("dir")!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
	emit workingDirectoryChanged();
      }
      if(getFromElement("geometry")
	 !=geometryOptions->value(geometryBox->currentText())) {
	emit changeDom(element, QString("geometry"), 
		       geometryOptions->value(geometryBox->currentText()));
      }
      if(getFromElement("closure")
	 !=closureOptions->value(closureBox->currentText()))
	{
	  emit changeDom(element, QString("closure"), 
			 closureOptions->value(closureBox->currentText()));
	}
      if(getFromElement("reflectivity")
	 !=reflectivityOptions->value(refBox->currentText())) {
	emit changeDom(element, QString("reflectivity"), 
		       reflectivityOptions->value(refBox->currentText()));
      }
      if(getFromElement("velocity")
	 !=velocityOptions->value(velBox->currentText())) {
	emit changeDom(element, QString("velocity"), 
		       velocityOptions->value(velBox->currentText()));
      }
      if(getFromElement("bottomlevel").toInt() !=bLBox->value()) {
	emit changeDom(element, QString("bottomlevel"), 
		       QString().setNum(bLBox->value()));
      }
      if(getFromElement("toplevel").toInt() !=tLBox->value()) {
	emit changeDom(element, QString("toplevel"), 
		       QString().setNum(tLBox->value()));
      }
      if(getFromElement("innerradius").toInt() !=iRBox->value()) {
	emit changeDom(element, QString("innerradius"), 
		       QString().setNum(iRBox->value()));
      }
      if(getFromElement("outerradius").toInt() !=oRBox->value()) {
	emit changeDom(element, QString("outerradius"), 
		       QString().setNum(oRBox->value()));
      }
      if(getFromElement("maxwavenumber").toInt() !=maxWaveNumBox->value()) {
	
	int box = maxWaveNumBox->value();
	int elem = getFromElement("maxwavenumber").toInt();

	emit changeDom(element, QString("maxwavenumber"), 
		       QString().setNum(maxWaveNumBox->value()));
	if(box < elem) {
	  QList<QDomElement> dataGapElements;
	  QDomElement dataGapChild = element.firstChildElement("maxdatagap");
	  while(!dataGapChild.isNull()&&
		(dataGapChild.tagName() == QString("maxdatagap"))) {
	    int waveNum = dataGapChild.attribute("wavenum").toInt();
	    if(waveNum > box) {
	      emit removeDom(element, QString("maxdatagap"), 
			     QString("wavenum"), QString().setNum(waveNum));
	    }
	    else {
	      emit changeDom(element, QString("maxdatagap"), 
			     QString().setNum(dataGapBoxes[waveNum]->value()),
			     QString("wavenum"), QString().setNum(waveNum));
	    }
	    dataGapChild = dataGapChild.nextSiblingElement("maxdatagap");
	  }
	}
	else {
	  for(int waveNum = elem+1; waveNum <= box; waveNum++) {
	    emit addDom(element, QString("maxdatagap"), 
			QString().setNum(dataGapBoxes[waveNum]->value()),
			QString("wavenum"), QString().setNum(waveNum));
	  }
	  for(int waveNum = 0; waveNum <= box; waveNum++) {
	    emit changeDom(element, QString("maxdatagap"),
			   QString().setNum(dataGapBoxes[waveNum]->value()),
			   QString("wavenum"), QString().setNum(waveNum));
	  }
	}
      }
      if(getFromElement("ringwidth").toDouble() !=ringBox->value()) {
	emit changeDom(element, QString("ringwidth"), 
		       QString().setNum(ringBox->value()));
      }
    }
  setPanelChanged(false);
  return true;
}

bool VTDPanel::setDefaultDirectory(QDir* newDir)
{
  QString subDirectory("vtd");
  if(!newDir->isAbsolute())
    newDir->makeAbsolute();
  if(newDir->exists(subDirectory))
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  if(newDir->mkdir(subDirectory)) {
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  }
  else {
    defaultDirectory = newDir;
    return false;
  }
  
}

HVVPPanel::HVVPPanel()
{
  //Do nothing
  setPanelChanged(false);
}

HVVPPanel::~HVVPPanel()
{
  // Will be implemented after constructor is
  // no parameters as of yet

}

void HVVPPanel::updatePanel(const QDomElement panelElement)
{
  setElement(panelElement);
  //Do nothing for now
  setPanelChanged(false);
}

bool HVVPPanel::updateConfig()
{
  if(checkPanelChanged())
    {
      //Do nothing
    }
  setPanelChanged(false);
  return true;
}


PressurePanel::PressurePanel()
{
  QLabel *dirLabel = new QLabel(tr("Directory Containing Pressure Data"));
  // dir = new QLineEdit();
  //defaultDirectory = new QDir(QDir::currentPath());
  if(!defaultDirectory->exists(defaultDirectory->filePath("pressure"))) {
	  defaultDirectory->mkdir("pressure") ;
  }
  defaultDirectory->cd("pressure");  
  //browse = new QPushButton(tr("Browse.."));
  dir->setText(defaultDirectory->path());
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *dirLayout = new QGridLayout;
  dirLayout->addWidget(dirLabel, 0, 0);
  dirLayout->addWidget(dir, 1, 0, 1, 3);
  dirLayout->addWidget(browse, 1,3);
  
  QLabel *pressureFormatLabel = new QLabel(tr("Data Format"));
  pressureFormatOptions = new QHash<QString, QString>;
  pressureFormatOptions->insert(QString("Select a Pressure Data Format"),
							 QString(""));
  pressureFormatOptions->insert(QString("Metar"), QString("METAR"));
  pressureFormatOptions->insert(QString("AWIPS"), QString("AWIPS"));
  pressureFormat = new QComboBox();
  QList<QString> options = pressureFormatOptions->keys();
  for(int i = 0; i < options.count(); i++) 
  {
      pressureFormat->addItem(options[i]);
  }
  pressureFormat->setEditable(false);
  QHBoxLayout *pressureFormatLayout = new QHBoxLayout;
  pressureFormatLayout->addWidget(pressureFormatLabel);
  pressureFormatLayout->addWidget(pressureFormat);
  // add more formats when found

  QLabel *maxObsTimeLabel = new QLabel(tr("Don't use pressure observations older than (minutes)"));
  maxObsTime = new QSpinBox;
  maxObsTime->setValue(59);
  maxObsTime->setRange(0,500);
  QHBoxLayout *maxObsTimeLayout = new QHBoxLayout;
  maxObsTimeLayout->addWidget(maxObsTimeLabel);
  maxObsTimeLayout->addWidget(maxObsTime);

  QGroupBox *maxObsDistBox = new QGroupBox(tr("Maximum distance to pressure observations"));
  maxObsDistCenter = new QRadioButton(tr("Measure from TC center"),maxObsDistBox);
 
  maxObsDistRing = new QRadioButton(tr("Measure from edge of analysis"),maxObsDistBox);
  
  QLabel *maxObsDistLabel = new QLabel(tr("Maximum distance (km)"));
  maxObsDist = new QDoubleSpinBox(maxObsDistBox);
  maxObsDist->setDecimals(2);
  maxObsDist->setRange(0,1000);
  maxObsDist->setValue(50);
  QHBoxLayout *maxObsDistLayout = new QHBoxLayout;
  maxObsDistLayout->addWidget(maxObsDistLabel);
  maxObsDistLayout->addWidget(maxObsDist);
 
  QVBoxLayout *maxObsDistBoxLayout = new QVBoxLayout;
  maxObsDistBoxLayout->addWidget(maxObsDistCenter);
  maxObsDistBoxLayout->addWidget(maxObsDistRing);
  maxObsDistBoxLayout->addLayout(maxObsDistLayout);
  maxObsDistBox->setLayout(maxObsDistBoxLayout);
  
  QVBoxLayout *main = new QVBoxLayout;
  main->addLayout(dirLayout);
  main->addLayout(pressureFormatLayout);
  main->addLayout(maxObsTimeLayout);
  main->addWidget(maxObsDistBox);
  main->addStretch(1);
  setLayout(main);

  connect(dir, SIGNAL(textChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(pressureFormat, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(maxObsTime, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(maxObsDist, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(maxObsDistCenter, SIGNAL(toggled(const bool)),
	  this, SLOT(valueChanged(const bool)));
  connect(maxObsDistRing, SIGNAL(toggled(const bool)),
	  this, SLOT(valueChanged(const bool)));

  setPanelChanged(false);
}

PressurePanel::~PressurePanel()
{
    delete pressureFormat;
    delete pressureFormatOptions;
    delete maxObsTime;
    delete maxObsDist;
    delete maxObsDistCenter;
    delete maxObsDistRing;
}

void PressurePanel::updatePanel(const QDomElement panelElement)
{
  // Sets the location of the panel's information in the Configuration
  // Iterates through all elements within this section of the Configuration
  // and writes the values of these parameters to the corresponding member
  // widget.

  setElement(panelElement);
  QDomElement child = panelElement.firstChildElement();
  while (!child.isNull()) {
    QString name = child.tagName();
    QString parameter = child.text();
    if (name == "dir") {
      if(parameter!=QString("default")) {
	dir->clear();
	dir->insert(parameter); 
	emit workingDirectoryChanged();}
      else 
	setPanelChanged(true);}
    if (name == "format") {
      int index = pressureFormat->findText(
		 pressureFormatOptions->key(parameter), 
		 Qt::MatchStartsWith);
      if (index != -1)
	pressureFormat->setCurrentIndex(index); }
    if(name == "maxobstime") {
      if(parameter.toInt()!=maxObsTime->value())
	maxObsTime->setValue(parameter.toInt()); }
    if(name == "maxobsdist") {
      if(parameter.toFloat()!=maxObsDist->value())
	maxObsDist->setValue(parameter.toFloat()); }
    if(name == "maxobsmethod"){
      if(parameter == "center"){
	maxObsDistCenter->setChecked(true);
      }
      if(parameter == "ring"){
	maxObsDistRing->setChecked(true);
      }
    }
    child = child.nextSiblingElement();
  }
  setPanelChanged(false);
}

bool PressurePanel::updateConfig()
{
  // If any the Panel's members have been changed these values will be
  // writen to the corresponding location within the Configuration

  QDomElement element = getPanelElement();
  if (checkPanelChanged())
    {
      if(getFromElement("dir")!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
	emit workingDirectoryChanged();
      }
      if(getFromElement("format")
	 !=pressureFormatOptions->value(pressureFormat->currentText()))
	{
	  emit changeDom(element, QString("format"), 
	    pressureFormatOptions->value(pressureFormat->currentText()));
	}
      if(getFromElement("maxobstime").toInt()!=maxObsTime->value()) {
	emit changeDom(element, QString("maxobstime"), 
		       QString().setNum(maxObsTime->value()));
      }
      if(getFromElement("maxobsdist").toFloat()!=maxObsDist->value()) {
	emit changeDom(element, QString("maxobsdist"), 
		       QString().setNum(maxObsDist->value()));
      }
      if(maxObsDistCenter->isChecked())
	emit changeDom(element, QString("maxobsmethod"), QString("center"));
      if(maxObsDistRing->isChecked())
	emit changeDom(element, QString("maxobsmethod"), QString("ring"));
    }
  setPanelChanged(false);
  return true;
}

bool PressurePanel::setDefaultDirectory(QDir* newDir)
{
  QString subDirectory("pressure");
  if(!newDir->isAbsolute())
    newDir->makeAbsolute();
  if(newDir->exists(subDirectory))
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  if(newDir->mkdir(subDirectory)) {
    if(newDir->cd(subDirectory)){
      if(newDir->isReadable()){
	defaultDirectory = newDir;
	return true;
      }
      else {
	newDir->cdUp();
	defaultDirectory = newDir;
	return false;
      }
    }
    else {
      defaultDirectory = newDir;
      return false;
    }
  }
  else {
    defaultDirectory = newDir;
    return false;
  }
}


GraphicsPanel::GraphicsPanel()
{

  graphParameters = new QGroupBox(tr("Parameters for Graph Display"));
  graphParameters->setCheckable(true);
  QGridLayout *graph = new QGridLayout;
  
  QLabel *pMax = new QLabel(tr("Maximum Pressure (mb)"));
  pMaxBox = new QDoubleSpinBox;
  pMaxBox->setRange(0,2000);
  pMaxBox->setDecimals(1);
  
  QLabel *pMin = new QLabel(tr("Minimum Pressure (mb)"));
  pMinBox = new QDoubleSpinBox;
  pMinBox->setRange(0, 2000);
  pMinBox->setDecimals(1);
  
  QLabel *rmwMax = new QLabel(tr("Maximum RMW (km)"));
  rmwMaxBox = new QDoubleSpinBox;
  rmwMaxBox->setDecimals(1);
  
  QLabel *rmwMin = new QLabel(tr("Minimum RMW (km)"));
  rmwMinBox = new QDoubleSpinBox;
  rmwMinBox->setDecimals(1);
  
  QLabel *beginTimeLabel = new QLabel(tr("Beginning Time"));
  startDateTime = new QDateTimeEdit();
  
  QLabel *endTimeLabel = new QLabel (tr("Endding Time"));
  endDateTime = new QDateTimeEdit();
  
  graph->addWidget(pMax, 0,0);
  graph->addWidget(pMaxBox, 0, 1);
  graph->addWidget(pMin, 1, 0);
  graph->addWidget(pMinBox, 1, 1);
  graph->addWidget(rmwMax, 0, 2);
  graph->addWidget(rmwMaxBox, 0, 3);
  graph->addWidget(rmwMin, 1, 2);
  graph->addWidget(rmwMinBox, 1, 3);
  graph->addWidget(beginTimeLabel, 2, 0);
  graph->addWidget(startDateTime, 2,1);
  graph->addWidget(endTimeLabel, 3, 0);
  graph->addWidget(endDateTime, 3,1);
  graphParameters->setLayout(graph);
  graphParameters->setChecked(false);
  QVBoxLayout *main = new QVBoxLayout;
  main->addWidget(graphParameters);
  main->addStretch(1);
  setLayout(main);

  connect(graphParameters, SIGNAL(toggled(bool)), 
	  this, SLOT(valueChanged(bool))); 
  connect(pMaxBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(pMinBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(rmwMaxBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(rmwMinBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(startDateTime, SIGNAL(dateTimeChanged(const QDateTime&)),
	  this, SLOT(valueChanged(const QDateTime&)));
  connect(endDateTime, SIGNAL(dateTimeChanged(const QDateTime&)), 
	  this, SLOT(valueChanged(const QDateTime&)));

  setPanelChanged(false);
}

GraphicsPanel::~GraphicsPanel()
{
  delete pMaxBox;
  delete pMinBox;
  delete rmwMaxBox;
  delete rmwMinBox;
  delete startDateTime;
  delete endDateTime;
  delete graphParameters;
}

void GraphicsPanel::updatePanel(const QDomElement panelElement)
{
  // Sets the location of the panel's information in the Configuration
  // Iterates through all elements within this section of the Configuration
  // and writes the values of these parameters to the corresponding member
  // widget.

  setElement(panelElement);
  QDomElement child = panelElement.firstChildElement();
  while (!child.isNull()) {
    QString name = child.tagName();
    QString parameter = child.text();
    if (name == "pressmin") {
      pMinBox->setValue(parameter.toDouble()); }
    if (name == "pressmax") {
      pMaxBox->setValue(parameter.toDouble()); }
    if (name == "rmwmin") {
      rmwMinBox->setValue(parameter.toDouble()); }
    if (name == "rmwmax") {
      rmwMaxBox->setValue(parameter.toDouble()); }
    if (name == "startdate") {
      startDateTime->setDate(QDate::fromString(parameter, "yyyy-MM-dd")); }
    if (name == "enddate") {
      endDateTime->setDate(QDate::fromString(parameter, "yyyy-MM-dd")); }
    if (name == "starttime") {
      startDateTime->setTime(QTime::fromString(parameter, "hh:mm:ss")); }
    if (name == "endtime") {
      endDateTime->setTime(QTime::fromString(parameter, "hh:mm:ss")); }
    child = child.nextSiblingElement();
  }
  setPanelChanged(false);
}

bool GraphicsPanel::updateConfig()
{
  // If any the Panel's members have been changed these values will be
  // writen to the corresponding location within the Configuration

  QDomElement element = getPanelElement();
  if (checkPanelChanged())
    {
      emit stateChange(QString("manualAxes"),graphParameters->isChecked()); 
      
      if(graphParameters->isChecked()) {
	emit changeDom(element, QString("pressmin"), 
		       QString().setNum(pMinBox->value()));

	emit changeDom(element, QString("pressmax"), 
		       QString().setNum(pMaxBox->value()));

	emit changeDom(element, QString("rmwmin"), 
		       QString().setNum(rmwMinBox->value()));

	emit changeDom(element, QString("rmwmax"), 
		       QString().setNum(rmwMaxBox->value()));

	emit changeDom(element, QString("startdate"), 
		       startDateTime->date().toString("yyyy-MM-dd"));

	emit changeDom(element, QString("enddate"), 
		       endDateTime->date().toString("yyyy-MM-dd"));

	emit changeDom(element, QString("starttime"), 
		       startDateTime->time().toString("hh:mm:ss"));
  
	emit changeDom(element, QString("endtime"), 
		       endDateTime->time().toString("hh:mm:ss"));
      }
    }
  setPanelChanged(false);
  return true;
}

QCPanel::QCPanel()
{
  QGroupBox *qcParameters = new QGroupBox(tr("Quality Control Parameters"));
  
  QLabel *velMinThresLabel = new QLabel(tr("Ignore Velocities With Magnitude Less Than (km/s)"));
  velocityMinimum = new QDoubleSpinBox;
  velocityMinimum->setDecimals(3);
  velocityMinimum->setRange(0,10);
  velocityMinimum->setValue(1);

  QLabel *velMaxThresLabel = new QLabel(tr("Ignore Velocities With Magnitude Greater Than (km/s)"));
  velocityMaximum = new QDoubleSpinBox;
  velocityMaximum->setDecimals(3);
  velocityMaximum->setRange(0,999);
  velocityMaximum->setValue(100);

  QLabel *refMinThresLabel = new QLabel(tr("Ignore Gates With Reflectivity Less Than (dBz)"));
  reflectivityMinimum = new QDoubleSpinBox;
  reflectivityMinimum->setDecimals(3);
  reflectivityMinimum->setRange(-500,500);
  reflectivityMinimum->setValue(-15);
 
  QLabel *refMaxThresLabel = new QLabel(tr("Ignore Gates With Reflectivity Greater Than (dBz)"));
  reflectivityMaximum = new QDoubleSpinBox;
  reflectivityMaximum->setDecimals(3);
  reflectivityMaximum->setRange(-500,500);
  reflectivityMaximum->setValue(65);

  QLabel *specThresLabel = new QLabel(tr("Ignore Gates With Spectral Width Greater Than"));
  spectralThreshold = new QDoubleSpinBox;
  spectralThreshold->setDecimals(2);
  spectralThreshold->setRange(0,50);
  spectralThreshold->setValue(10);
  
  QLabel *bbLabel = new QLabel(tr("Number of Gates Averaged for Velocity Dealiasing"));
  bbSegmentSize = new QSpinBox;
  bbSegmentSize->setRange(1,150);
  bbSegmentSize->setValue(50);

  QLabel *maxFoldLabel = new QLabel(tr("Maximum Number of Folds in Velocity Dealiasing"));
  maxFoldCount = new QSpinBox;
  maxFoldCount->setRange(0,100);
  maxFoldCount->setValue(4);

  QGridLayout *paramLayout = new QGridLayout;
  paramLayout->addWidget(velMinThresLabel,0,0,1,3);
  paramLayout->addWidget(velocityMinimum,0,2,1,1);
  paramLayout->addWidget(velMaxThresLabel,1,0,1,3);
  paramLayout->addWidget(velocityMaximum, 1,2,1,1);
  paramLayout->addWidget(refMinThresLabel,2,0,1,3);
  paramLayout->addWidget(reflectivityMinimum,2,2,1,1);
  paramLayout->addWidget(refMaxThresLabel, 3,0,1,3);
  paramLayout->addWidget(reflectivityMaximum, 3,2,1,1);
  paramLayout->addWidget(specThresLabel,4,0,1,3);
  paramLayout->addWidget(spectralThreshold,4,2,1,1);
  paramLayout->addWidget(bbLabel,5,0,1,3);
  paramLayout->addWidget(bbSegmentSize,5,2,1,1);
  paramLayout->addWidget(maxFoldLabel,6,0,1,3);
  paramLayout->addWidget(maxFoldCount,6,2,1,1);
  qcParameters->setLayout(paramLayout);

  QGroupBox *findWind = new QGroupBox(tr("Method For Finding Reference Wind"));
  vad = new QRadioButton(tr("Use GVAD and VAD Algorithms"), findWind);
  user = new QRadioButton(tr("Enter Environmental Wind Parameters"), findWind);
  known = new QRadioButton(tr("Use Available AWEPS Data"), findWind);

  QFrame *vadParameters = new QFrame;

  QLabel *vadLevelsLabel = new QLabel(tr("Number of VAD Levels Used"));
  vadLevels = new QSpinBox;
  vadLevels->setRange(5,50);
  vadLevels->setValue(20);
  QHBoxLayout *vadLevelsLayout = new QHBoxLayout;
  vadLevelsLayout->addSpacing(20);
  vadLevelsLayout->addWidget(vadLevelsLabel, 1);
  vadLevelsLayout->addWidget(vadLevels);

  QLabel *numCoLabel=new QLabel(tr("Number of Coefficients Used in VAD Fits"));
  numCoefficients = new QSpinBox;
  numCoefficients->setRange(3,5);
  numCoefficients->setValue(3);
  QHBoxLayout *numCoLayout = new QHBoxLayout;
  numCoLayout->addSpacing(20);
  numCoLayout->addWidget(numCoLabel,1);
  numCoLayout->addWidget(numCoefficients);

  QLabel *vadthrLabel = new QLabel(tr("Minimum number of points for VAD processing"));
  vadthr = new QSpinBox;
  vadthr->setRange(0,360);
  vadthr->setValue(30);
  QHBoxLayout *vadthrLayout = new QHBoxLayout;
  vadthrLayout->addSpacing(20);
  vadthrLayout->addWidget(vadthrLabel);
  vadthrLayout->addWidget(vadthr);

QLabel *gvadthrLabel = new QLabel(tr("Minimum number of points for GVAD processing"));
  gvadthr = new QSpinBox;
  gvadthr->setRange(0,360);
  gvadthr->setValue(30);
  QHBoxLayout *gvadthrLayout = new QHBoxLayout;
  gvadthrLayout->addSpacing(20);
  gvadthrLayout->addWidget(gvadthrLabel);
  gvadthrLayout->addWidget(gvadthr);

  QVBoxLayout *vadLayout = new QVBoxLayout;
  vadLayout->addLayout(vadLevelsLayout);
  vadLayout->addLayout(numCoLayout);
  vadLayout->addLayout(vadthrLayout);
  vadLayout->addLayout(gvadthrLayout);
  vadParameters->setLayout(vadLayout);
  vadParameters->hide();

  QFrame *userParameters = new QFrame;
 
  QLabel *windSpeedLabel = new QLabel(tr("Wind Speed (km/s)"));
  windSpeed = new QDoubleSpinBox;
  windSpeed->setDecimals(2);
  windSpeed->setRange(0, 200);
  QHBoxLayout *windSpeedLayout = new QHBoxLayout;
  windSpeedLayout->addSpacing(20);
  windSpeedLayout->addWidget(windSpeedLabel);
  windSpeedLayout->addWidget(windSpeed);

  QLabel *windDirectionLabel = new QLabel(tr("Wind Direction (degrees from North)"));
  windDirection = new QDoubleSpinBox;
  windDirection->setDecimals(1);
  windDirection->setRange(0, 359.9);
  QHBoxLayout *windDirectionLayout = new QHBoxLayout;
  windDirectionLayout->addSpacing(20);
  windDirectionLayout->addWidget(windDirectionLabel);
  windDirectionLayout->addWidget(windDirection);

  QVBoxLayout *userParametersLayout = new QVBoxLayout;
  userParametersLayout->addLayout(windSpeedLayout);
  userParametersLayout->addLayout(windDirectionLayout);
  userParameters->setLayout(userParametersLayout);
  userParameters->hide();

  QFrame *knownParameters = new QFrame;
  QLabel *knownDirLabel = new QLabel(tr("AWIPS Data Directory"));
  // dir = new QLineEdit();
  //defaultDirectory = new QDir(QDir::currentPath());
  //browse = new QPushButton("Browse..");
  dir->setText(defaultDirectory->path());
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *knownDirLayout = new QGridLayout();
  QHBoxLayout *knownLayout = new QHBoxLayout;
  knownDirLayout->addWidget(knownDirLabel, 0, 0);
  knownDirLayout->addWidget(dir, 1, 0, 1, 3);
  knownDirLayout->addWidget(browse, 1, 3);
  knownLayout->addSpacing(20);
  knownLayout->addLayout(knownDirLayout);
  knownParameters->setLayout(knownLayout);
  knownParameters->hide();

  QVBoxLayout *findWindLayout = new QVBoxLayout;
  findWindLayout->addWidget(vad);
  findWindLayout->addWidget(vadParameters);
  findWindLayout->addWidget(user);
  findWindLayout->addWidget(userParameters);
  findWindLayout->addWidget(known);
  findWindLayout->addWidget(knownParameters);
  findWind->setLayout(findWindLayout);

  QVBoxLayout *main = new QVBoxLayout;
  main->addWidget(qcParameters);
  main->addWidget(findWind);
  main->addStretch(1);
  setLayout(main);

  connect(bbSegmentSize, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(maxFoldCount, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(velocityMinimum, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(velocityMaximum, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(reflectivityMinimum, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(reflectivityMaximum, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(spectralThreshold, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));

  connect(vad, SIGNAL(toggled(const bool)), 
	  this, SLOT(valueChanged(const bool)));
  connect(vad, SIGNAL(toggled(bool)),
	  vadParameters, SLOT(setVisible(bool)));
  connect(vadLevels, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(numCoefficients, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(vadthr, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(gvadthr, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));

  connect(user, SIGNAL(toggled(const bool)), 
	  this, SLOT(valueChanged(const bool)));
  connect(user, SIGNAL(toggled(bool)), 
	  userParameters, SLOT(setVisible(bool)));
  connect(windSpeed, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  connect(windDirection, SIGNAL(valueChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));

  connect(known, SIGNAL(toggled(const bool)), 
	  this, SLOT(valueChanged(const bool)));
  connect(known, SIGNAL(toggled(bool)),
	  knownParameters, SLOT(setVisible(bool)));
  connect(dir, SIGNAL(textChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));
  
  setPanelChanged(false);

}

QCPanel::~QCPanel()
{
  delete vad;
  delete user;
  delete known;
  delete bbSegmentSize;
  delete maxFoldCount;
  delete vadLevels;
  delete numCoefficients;
  delete vadthr;
  delete gvadthr;
  delete velocityMinimum;
  delete velocityMaximum;
  delete spectralThreshold;
  delete reflectivityMinimum;
  delete reflectivityMaximum;
  delete windSpeed;
  delete windDirection;
}

void QCPanel::updatePanel(const QDomElement panelElement)
{
  setElement(panelElement);
  QDomElement child = panelElement.firstChildElement();
  while (!child.isNull()) {
    QString name = child.tagName();
    QString parameter = child.text();
    if(name == "wind_method") {
      if(parameter == "vad")
	vad->setChecked(true);
      if(parameter == "user")
	user->setChecked(true);
      if(parameter == "known")
	known->setChecked(true); }
    if(name == "vel_min") {
      velocityMinimum->setValue(parameter.toDouble()); }
    if(name == "vel_max") {
      velocityMaximum->setValue(parameter.toDouble()); }
    if(name == "ref_min") {
      reflectivityMinimum->setValue(parameter.toDouble()); }
    if(name == "ref_max") {
      reflectivityMaximum->setValue(parameter.toDouble()); }
    if(name == "sw_threshold") {
      spectralThreshold->setValue(parameter.toDouble()) ; }
    if(name == "bbcount") {
      bbSegmentSize->setValue(parameter.toInt()); }
    if(name == "maxfold") {
      maxFoldCount->setValue(parameter.toInt()); }
    if(name == "windspeed") {
      windSpeed->setValue(parameter.toDouble()); }
    if(name == "winddirection") {
      windDirection->setValue(parameter.toDouble()); }
    if(name == "awips_dir") {
      dir->clear();
      dir->insert(parameter); }
    if(name == "vadlevels") {
      vadLevels->setValue(parameter.toInt()); }
    if(name == "numcoeff") {
      numCoefficients->setValue(parameter.toInt()); }
    if(name == "vadthr") {
      vadthr->setValue(parameter.toInt()); }
    if(name == "gvadthr") {
      gvadthr->setValue(parameter.toInt()); }
    child = child.nextSiblingElement();
  }
  setPanelChanged(false);
}

bool QCPanel::updateConfig()
{
  QDomElement element = getPanelElement();
  if (checkPanelChanged()) 
    {
      if(getFromElement("vel_min").toDouble()!=velocityMinimum->value()) {
	emit changeDom(element, QString("vel_min"), 
		       QString().setNum(velocityMinimum->value()));
      }
      if(getFromElement("vel_max").toDouble() !=velocityMaximum->value()) {
	emit changeDom(element, QString("vel_max"), 
		       QString().setNum(velocityMaximum->value()));
      }
      if(getFromElement("ref_min").toDouble()!=reflectivityMinimum->value()) {
	emit changeDom(element, QString("ref_min"), 
		       QString().setNum(reflectivityMinimum->value()));
      }
      if(getFromElement("ref_max").toDouble() !=reflectivityMaximum->value()) {
	emit changeDom(element, QString("ref_max"), 
		       QString().setNum(reflectivityMaximum->value()));
      }
      if(getFromElement("sw_threshold").toDouble()!=spectralThreshold->value()) {
	emit changeDom(element, QString("sw_threshold"),
		       QString().setNum(spectralThreshold->value()));
      }
      if(getFromElement("bbcount").toInt() !=bbSegmentSize->value()) {
	emit changeDom(element,QString("bbcount"),
		       QString().setNum(bbSegmentSize->value()));
      }
      if(getFromElement("maxfold").toInt()!=maxFoldCount->value()) {
	emit changeDom(element, QString("maxfold"),
		       QString().setNum(maxFoldCount->value()));
      }
      if(vad->isChecked()) {
	emit changeDom(element, QString("wind_method"), QString("vad"));
	
	if(getFromElement("vadlevels").toInt()!= vadLevels->value()) {
	  emit changeDom(element, QString("vadlevels"),
			  QString().setNum(vadLevels->value()));
	}
	if(getFromElement("numcoeff").toInt()!= numCoefficients->value()) {
	  emit changeDom(element, QString("numcoeff"),
			 QString().setNum(numCoefficients->value()));
	}
	if(getFromElement("vadthr").toInt()!=vadthr->value()) {
	  emit changeDom(element, QString("vadthr"),
			 QString().setNum(vadthr->value()));
	}
	if(getFromElement("gvadthr").toInt()!=gvadthr->value()) {
	  emit changeDom(element, QString("gvadthr"),
			 QString().setNum(gvadthr->value()));
	}
      }
      if(user->isChecked()) {
	emit changeDom(element, QString("wind_method"), QString("user"));
	
	if(getFromElement("windspeed").toDouble()!= windSpeed->value()) {
	  emit changeDom(element, QString("windspeed"),
			  QString().setNum(windSpeed->value()));
	}
	if(getFromElement("winddirection").toDouble()!=windDirection->value()){
	  emit changeDom(element, QString("winddirection"),
			 QString().setNum(windDirection->value()));
	}
      }
      if(known->isChecked()) {
	emit changeDom(element, QString("wind_method"), QString("known"));
	
	if(getFromElement("awips_dir")!= dir->text()) {
	  emit changeDom(element, QString("awips_dir"),dir->text());
	}
      }
      setPanelChanged(false);
    }
  return true;
}

/*
  void RadarPanel::fillRadarHash()
  {

  
    Was used before the xml file for radar information was created
    This is kept on hand for now incase the info in the xml is overwritten
    
    radarNameOptions = new QHash<QString, QString>;
    radarLocations = new QHash<QString, QPointF>;
    radarNameOptions->insert(QString("Please select a radar"), QString(""));
    radarLocations->insert(QString("Please select a radar"), QPointF(0.0,0.0));
    radarNameOptions->insert(QString("RCWF in Taiwan"),QString("RCWF"));
    radarLocations->insert(QString("RCWF in Taiwan"), 
                           QPointF(121.773, 25.073));
    radarLocations->insert(QString("TJUA in San Juan Puerto Rico"), 
    QPointF(-66.078, 18.116));
    radarLocations->insert(QString("KENX in Albany, NY"), 
    QPointF(-74.064, 42.586));
    radarLocations->insert(QString("Anderson_AFB in Guam"), 
    QPointF(144.811,13.453));
    radarLocations->insert(QString("KFFC in Atlanta, GA"),
    QPointF(-84.567, 33.364));
    radarLocations->insert(QString("KEWX in Austin/San Antonio, TX"),
    QPointF(-98.028, 29.704));
    radarLocations->insert(QString("KBGM in Binghamton, NY"),
    QPointF(-75.985, 42.200));
    radarLocations->insert(QString("KBMX in Birmingham, AL"),
    QPointF(-86.770, 33.172));
    radarLocations->insert(QString("KBOX in Boston, MA"), 
    QPointF(-71.137, 41.956));
    radarLocations->insert(QString("KBRO in Brownsville, TX"),
    QPointF(-97.419, 25.916));
    radarLocations->insert(QString("RKSG in Camp Humphreys, South Korea"),
    QPointF(127.021, 36.956));
    radarLocations->insert(QString("KCLX in Charleston, SC"),
    QPointF(-81.042, 32.656));
    radarLocations->insert(QString("KCAE in Columbia SC"),
    QPointF(-81.118, 33.949));
    radarLocations->insert(QString("KCRP in Corpus Christi, TX"),
    QPointF(-97.511,27.784));
    radarLocations->insert(QString("KDOX at Dover AFB, DE"),
    QPointF(-75.440,38.826));
    radarLocations->insert(QString("KDYX at Dyess AFB, TX"),
    QPointF(-99.254, 32.538));
    radarLocations->insert(QString("KEVX at Eglin AFB, FL"),
    QPointF(-85.921, 30.564));
    radarLocations->insert(QString("KPOE at Fort Polk, LA"),
    QPointF(-92.976, 31.156));
    radarLocations->insert(QString("KCBW in Houlton, ME"),
    QPointF(-67.806, 46.039));
    radarLocations->insert(QString("KHGX in Galveston, TX"),
    QPointF(-95.079, 29.742));
    radarLocations->insert(QString("KJAN in Jackson, MS"),
    QPointF(-90.080,32.318));
    radarLocations->insert(QString("KJAX in JacksonVille, FL"),
    QPointF(81.701, 30.485));
    radarLocations->insert(QString("RODN in Kadena Okinawa, Japan"),
    QPointF(127.910, 26.302));
    radarLocations->insert(QString("KBYX in Key West, FL"),
    QPointF(-81.703, 24.598));
    radarLocations->insert(QString("KLCH in Lake Charles, LA"),
    QPointF(-93.216, 30.125));
    radarLocations->insert(QString("KMXX in Maxwell ARB, AL"),
    QPointF(-85.790, 32.537));
    radarLocations->insert(QString("KMLB in Melbourne, FL"),
    QPointF(-80.654, 28.113));
    radarLocations->insert(QString("KAMX in Miami, FL"),
    QPointF(-80.413, 25.611));
    radarLocations->insert(QString("KMOB in Mobile, AL"),
    QPointF(-88.240, 30.679));
    radarLocations->insert(QString("KVAX at Moody AFB, GA"),
    QPointF(-83.390, 30.390));
    radarLocations->insert(QString("KMHX in Morehead City, NC"),
    QPointF(-76.876, 34.776));
    radarLocations->insert(QString("KLIX in New Orleans, LA"),
    QPointF(-89.826, 30.337));
    radarLocations->insert(QString("KOKX in New York City, NY"),
    QPointF(-72.864, 40.866));
    radarLocations->insert(QString("KAKO, in Richmond, VA"),
    QPointF(-77.007, 36.984));
    radarLocations->insert(QString("KRAX in Raleigh, NC"),
    QPointF(-78.490, 35.666));
    radarLocations->insert(QString("PHKI in South Kauai, HI"),
    QPointF(-159.552, 21.894));
    radarLocations->insert(QString("KLWX in Sterling, VA"),
    QPointF(-77.478, 38.975));
    radarLocations->insert(QString("KTLH in Tallahassee, FL"),
    QPointF(-84.329, 30.398));
    radarLocations->insert(QString("KTBW in Tampa, FL"),
    QPointF(-82.402, 27.706 ));
    radarLocations->insert(QString("KLTX in Wilmington, NC"),
    QPointF(-78.429, 33.990));
    radarLocations->insert(QString("Other Radar...."),
    QPointF(0.0, 0.0));
    }
*/
