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
  QRadioButton *tDepression = new QRadioButton(tr("Tropical Depression"), stormType);
  QVBoxLayout *stormTypeLayout = new QVBoxLayout;
  stormTypeLayout->addWidget(hurricane);
  stormTypeLayout->addWidget(tStorm);
  stormTypeLayout->addWidget(tDepression);
  stormType->setLayout(stormTypeLayout);

  QLabel *latLabel = new QLabel(tr("Vortex Latitude:"));
  latBox = new QDoubleSpinBox();
  latBox->setRange(-999,999);
  QHBoxLayout *lat = new QHBoxLayout;
  lat->addWidget(latLabel);
  lat->addWidget(latBox);
      
  QLabel *longLabel = new QLabel(tr("Vortex Longitude"));
  longBox = new QDoubleSpinBox();
  longBox->setRange(-999, 999);
  QHBoxLayout *longitude = new QHBoxLayout;
  longitude->addWidget(longLabel);
  longitude->addWidget(longBox);

  QLabel *workingDirLabel = new QLabel(tr("Working Directory"));
  dir = new QLineEdit();
  browse = new QPushButton("Browse..");
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
  layout->addLayout(dirLayout);
  layout->addStretch(1);
  setLayout(layout);
  
  connect(vortexName, SIGNAL(textChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(latBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(longBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(dir, SIGNAL(textChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));

  setPanelChanged(false);
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
      if(name == "dir")  {
	dir->clear();
	dir->insert(parameter); }
      
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
      if(element.firstChildElement("name").text()
	 !=vortexName->text()) {
	emit changeDom(element, "name", vortexName->text());
      }
      if(element.firstChildElement("lat").text().toDouble()
	 !=latBox->value()) {
	emit changeDom(element, QString("lat"), 
		       QString().setNum(latBox->value()));
      }
      if(element.firstChildElement("lon").text().toDouble()
	 !=longBox->value()) {
	emit changeDom(element, QString("lon"), 
		       QString().setNum(longBox->value()));
      }
      if(element.firstChildElement("dir").text()!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
      }
      
    }
  setPanelChanged(false);
  return true;
}

RadarPanel::RadarPanel()
{
  QLabel *radarNameLabel = new QLabel(tr("Radar Name:"));
  radarName = new QComboBox();
  radars = new Configuration(this,QString("vortrac_radarList.xml"));
  connect(radars, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
  radars->read(QString("vortrac_radarList.xml"));
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
  radarLatBox = new QDoubleSpinBox;
  radarLatBox->setRange(-999,999);
  radarLatBox->setDecimals(3);
  QHBoxLayout *lat = new QHBoxLayout;
  lat->addWidget(latLabel);
  lat->addWidget(radarLatBox);

  QLabel *longLabel = new QLabel(tr("Radar Longitude:"));
  radarLongBox = new QDoubleSpinBox();
  radarLongBox->setDecimals(3);
  radarLongBox->setRange(-999, 999);
  QHBoxLayout *longitude = new QHBoxLayout;
  longitude->addWidget(longLabel);
  longitude->addWidget(radarLongBox);

  QLabel *altLabel = new QLabel(tr("Radar Altitude in meters:"));
  radarAltBox = new QDoubleSpinBox();
  radarAltBox->setDecimals(3);
  radarAltBox->setRange(-999, 999);
  QHBoxLayout *altitude = new QHBoxLayout;
  altitude->addWidget(altLabel);
  altitude->addWidget(radarAltBox);

  QLabel *radarDirLabel = new QLabel(tr("Radar Data Directory"));
  dir = new QLineEdit();
  browse = new QPushButton("Browse..");
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *radarDirLayout = new QGridLayout();
  radarDirLayout->addWidget(radarDirLabel, 0, 0);
  radarDirLayout->addWidget(dir, 1, 0, 1, 3);
  radarDirLayout->addWidget(browse, 1, 3);

  QLabel *radarFormatLabel = new QLabel(tr("Data Format"));
  radarFormatOptions = new QHash<QString, QString>;
  radarFormatOptions->insert(QString("Select a Radar Data Format"),
			     QString(""));
  radarFormatOptions->insert(QString("Level II"), QString("LEVELII"));
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
      dir->clear();
      dir->insert(parameter); }
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
      if(element.firstChildElement("name").text()!=
	 radarName->currentText().left(4)) {
	emit changeDom(element, "name", 
		       radarName->currentText().left(4));
      }
      if(element.firstChildElement("lat").text().toDouble()
	 !=radarLatBox->value()) {
	emit changeDom(element, QString("lat"), 
		       QString().setNum(radarLatBox->value()));
      }
      if(element.firstChildElement("lon").text().toDouble()
	 !=radarLongBox->value()) {
	emit changeDom(element, QString("lon"), 
		       QString().setNum(radarLongBox->value()));
      }
      if(element.firstChildElement("alt").text().toFloat()
	 !=radarAltBox->value()) {
	emit changeDom(element, QString("alt"),
		       QString().setNum(radarAltBox->value()));
      }
      if(element.firstChildElement("dir").text()!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
      }
      if(element.firstChildElement("startdate").text()
	 !=startDateTime->date().toString("yyyy-MM-dd")) {
	emit changeDom(element, QString("startdate"), 
		       startDateTime->date().toString("yyyy-MM-dd"));
      }
      if(element.firstChildElement("enddate").text()
	 !=endDateTime->date().toString("yyyy-MM-dd")) {
	emit changeDom(element, QString("enddate"), 
		       endDateTime->date().toString("yyyy-MM-dd"));
      }
      if(element.firstChildElement("starttime").text()
	 !=startDateTime->time().toString("hh:mm:ss")) {
	emit changeDom(element, QString("starttime"), 
		       startDateTime->time().toString("hh:mm:ss"));
      }
      if(element.firstChildElement("endtime").text()
	 !=endDateTime->time().toString("hh:mm:ss")) {
	emit changeDom(element, QString("endtime"), 
		       endDateTime->time().toString("hh:mm:ss"));
      }
      if(element.firstChildElement("format").text()
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
  dir = new QLineEdit;
  browse = new QPushButton(tr("Browse.."));
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *cappiDir = new QGridLayout;
  cappiDir->addWidget(cappiDirLabel, 0, 0);
  cappiDir->addWidget(dir, 1, 0, 1, 3);
  cappiDir->addWidget(browse, 1,3);

  QGroupBox *grid = new QGroupBox(tr("Griding Configurations"));

  QLabel *xdim = new QLabel(tr("Length of Grid in X Direction"));
  QLabel *ydim = new QLabel(tr("Length of Grid in Y Direction"));
  QLabel *zdim = new QLabel(tr("Length of Grid in Z Direction"));
  xDimBox = new QDoubleSpinBox;
  xDimBox->setDecimals(1);
  xDimBox->setRange(-999,999);
  yDimBox = new QDoubleSpinBox;
  yDimBox->setDecimals(1);
  yDimBox->setRange(-999,999);
  zDimBox = new QDoubleSpinBox;
  zDimBox->setDecimals(1);
  zDimBox->setRange(-999,999);

  QLabel *xGrid = new QLabel(tr("X Grid Spacing"));
  xGridBox = new QDoubleSpinBox;
  xGridBox->setDecimals(1);

  QLabel *yGrid = new QLabel(tr("Y Grid Spacing"));
  yGridBox = new QDoubleSpinBox;
  yGridBox->setDecimals(1);

  QLabel *zGrid = new QLabel(tr("Z Grid Spacing"));
  zGridBox = new QDoubleSpinBox;
  zGridBox->setDecimals(1);

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

  QGroupBox *MaxMin = new QGroupBox(tr("Threshold Extrema"));

  QLabel *refMin = new QLabel(tr("Reflectivity Minimum"));
  refMinBox = new QDoubleSpinBox;
  refMinBox->setDecimals(1);
  refMinBox->setRange(-100,100);

  QLabel *refMax = new QLabel(tr("Reflectivity Maximum"));
  refMaxBox = new QDoubleSpinBox;
  refMaxBox->setDecimals(1);
  refMaxBox->setRange(-100,100);

  QLabel *velMin = new QLabel(tr("Velocity Minimum"));
  velMinBox = new QDoubleSpinBox;
  velMinBox->setDecimals(1);
  velMinBox->setRange(-500, 500);

  QLabel *velMax = new QLabel(tr("Velocity Maximum"));
  velMaxBox = new QDoubleSpinBox;
  velMaxBox->setDecimals(1);
  velMaxBox->setRange(-500,500);

  QGridLayout *minmax = new QGridLayout;
  minmax->addWidget(refMin, 0,0);
  minmax->addWidget(refMinBox, 0, 1);
  minmax->addWidget(refMax, 1, 0);
  minmax->addWidget(refMaxBox, 1, 1);
  minmax->addWidget(velMin, 0, 2);
  minmax->addWidget(velMinBox, 0, 3);
  minmax->addWidget(velMax, 1, 2);
  minmax->addWidget(velMaxBox, 1, 3);
  MaxMin->setLayout(minmax);

  QLabel *advSpeedLabel = new QLabel(tr("Advection Speed"));
  advSpeedBox = new QDoubleSpinBox;
  advSpeedBox->setDecimals(1);
  QLabel *advDirLabel = new QLabel(tr("Advection Direction"));
  advDirBox = new QDoubleSpinBox;
  advDirBox->setDecimals(1);
  QHBoxLayout *adv = new QHBoxLayout;
  adv->addWidget(advSpeedLabel);
  adv->addWidget(advSpeedBox);
  adv->addWidget(advDirLabel);
  adv->addWidget(advDirBox);

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
  layout->addWidget(MaxMin);
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
  connect(refMinBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(refMaxBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(velMinBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(velMaxBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(advSpeedBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&))); 
  connect(advDirBox, SIGNAL(valueChanged(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  connect(intBox, SIGNAL(activated(const QString&)), 
	  this, SLOT(valueChanged(const QString&)));
  // These connections allow individual widgets to notify the panel when 
  // a parameter has changed.

  setPanelChanged(false);
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
      dir->clear();
      dir->insert(parameter); }
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
    if (name == "ref_min") {
      refMinBox->setValue(parameter.toDouble()); }
    if (name == "ref_max") {
      refMaxBox->setValue(parameter.toDouble()); }
    if (name == "vel_min") {
      velMinBox->setValue(parameter.toDouble()); }
    if (name == "vel_max") {
      velMaxBox->setValue(parameter.toDouble()); }
    if (name == "adv_speed") {
      advSpeedBox->setValue(parameter.toDouble()); }
    if( name == "adv_dir") {
      advDirBox->setValue(parameter.toDouble()); }
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
      if(element.firstChildElement("dir").text()!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
  }
      if(element.firstChildElement("xdim").text().toDouble()
	 !=xDimBox->value()) {
	emit changeDom(element, QString("xdim"), 
		       QString().setNum(xDimBox->value()));
      }
      if(element.firstChildElement("ydim").text().toDouble()
	 !=yDimBox->value()) {
	emit changeDom(element, QString("ydim"), 
		       QString().setNum(yDimBox->value()));
      }
      if(element.firstChildElement("zdim").text().toDouble()
	 !=zDimBox->value()) {
    emit changeDom(element, QString("zdim"), 
		   QString().setNum(zDimBox->value()));
      }
      if(element.firstChildElement("xgridsp").text().toDouble()
	 !=xGridBox->value()) {
	emit changeDom(element, QString("xgridsp"), 
		       QString().setNum(xGridBox->value()));
      }
      if(element.firstChildElement("ygridsp").text().toDouble()
	 !=yGridBox->value()) {
	emit changeDom(element, QString("ygridsp"), 
		       QString().setNum(yGridBox->value()));
      }
      if(element.firstChildElement("zgridsp").text().toDouble()
	 !=zGridBox->value()) {
	emit changeDom(element, QString("zgridsp"), 
		       QString().setNum(zGridBox->value()));
      }
      if(element.firstChildElement("ref_min").text().toDouble()
	 !=refMinBox->value()) {
	emit changeDom(element, QString("ref_min"), 
		       QString().setNum(refMinBox->value()));
      }
      if(element.firstChildElement("ref_max").text().toDouble()
	 !=refMaxBox->value()) {
	emit changeDom(element, QString("ref_max"), 
		       QString().setNum(refMaxBox->value()));
      }
      if(element.firstChildElement("vel_min").text().toDouble()
	 !=velMinBox->value()) {
	emit changeDom(element, QString("vel_min"), 
		       QString().setNum(velMinBox->value()));
      }
      if(element.firstChildElement("vel_max").text().toDouble()
	 !=velMaxBox->value()) {
	emit changeDom(element, QString("vel_max"), 
		       QString().setNum(velMaxBox->value()));
      }
      if(element.firstChildElement("adv_speed").text().toDouble()
	 !=advSpeedBox->value()) {
	emit changeDom(element, QString("adv_speed"), 
		       QString().setNum(advSpeedBox->value()));
      }
      if(element.firstChildElement("adv_dir").text().toDouble()
	 !=advDirBox->value()) {
	emit changeDom(element, QString("adv_dir"), 
		       QString().setNum(advDirBox->value()));
      }
      if(element.firstChildElement("interpolation").text()
	 !=interpolationMethod->value(intBox->currentText())) 
	{
	  emit changeDom(element, QString("interpolation"),
			 interpolationMethod->value(intBox->currentText()));
	}
    }
  setPanelChanged(false);
  return true;
}

CenterPanel::CenterPanel()
{
  QLabel *dirLabel = new QLabel(tr("Center Output Directory"));
  dir = new QLineEdit();
  browse = new QPushButton(tr("Browse.."));
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
  QLabel *bottomLevel = new QLabel(tr("Bottom Level"));
  bLBox = new QSpinBox;
  QLabel *topLevel = new QLabel(tr("Top Level"));
  tLBox = new QSpinBox;
  QLabel *innerRad = new QLabel(tr("Inner Radius"));
  iRBox = new QSpinBox;
  QLabel *outerRad = new QLabel(tr("Outer Radius"));
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
  maxWaveNumBox = new QSpinBox;
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

  QLabel *ringWidth = new QLabel(tr("Width of Search Rings"));
  ringBox = new QDoubleSpinBox;
  ringBox->setDecimals(1);

  QLabel *influenceRadius = new QLabel(tr("Radius of Influence"));
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
      dir->clear();
      dir->insert(parameter); }
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
      if(parameter.toInt() != dataGapBoxes.count()-1)
	{
	  createDataGaps();
	}
      for(int i = 0; i <dataGapBoxes.count(); i++)
	{
	  QString tagname("maxdatagap_"+QString().setNum(i));
	  double dataGap = panelElement.firstChildElement(tagname).text().toDouble();
	  dataGapBoxes[i]->setValue(dataGap);
	}
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
      if(element.firstChildElement("dir").text()!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
      }
      if(element.firstChildElement("geometry").text()
	 !=geometryOptions->value(geometryBox->currentText())) {
	emit changeDom(element, QString("geometry"), 
		       geometryOptions->value(geometryBox->currentText()));
      }
      if(element.firstChildElement("closure").text()
	 !=closureOptions->value(closureBox->currentText()))
	{
	  emit changeDom(element, QString("closure"), 
			 closureOptions->value(closureBox->currentText()));
	}
      if(element.firstChildElement("reflectivity").text()
	 !=reflectivityOptions->value(refBox->currentText())) {
	emit changeDom(element, QString("reflectivity"), 
		       reflectivityOptions->value(refBox->currentText()));
      }
      if(element.firstChildElement("velocity").text()
	 !=velocityOptions->value(velBox->currentText())) {
	emit changeDom(element, QString("velocity"), 
		       velocityOptions->value(velBox->currentText()));
      }
      if(element.firstChildElement("search").text()
	 !=criteriaOptions->value(critBox->currentText())) {
	emit changeDom(element, QString("search"), 
		       criteriaOptions->value(critBox->currentText()));
      }
      if(element.firstChildElement("bottomlevel").text().toInt()
	 !=bLBox->value()) {
	emit changeDom(element, QString("bottomlevel"), 
		       QString().setNum(bLBox->value()));
      }
      if(element.firstChildElement("toplevel").text().toInt()
	 !=tLBox->value()) {
	emit changeDom(element, QString("toplevel"), 
		       QString().setNum(tLBox->value()));
      }
      if(element.firstChildElement("innerradius").text().toInt()
	 !=iRBox->value()) {
	emit changeDom(element, QString("innerradius"), 
		       QString().setNum(iRBox->value()));
      }
      if(element.firstChildElement("outerradius").text().toInt()
	 !=oRBox->value()) {
	emit changeDom(element, QString("outerradius"), 
		       QString().setNum(oRBox->value()));
      }
      if(element.firstChildElement("maxwavenumber").text().toInt()
	 !=maxWaveNumBox->value()) {

	int box = maxWaveNumBox->value();
	int elem = element.firstChildElement("maxwavenumber").text().toInt();

	emit changeDom(element, QString("maxwavenumber"), 
		       QString().setNum(maxWaveNumBox->value()));
	
	if (box<elem) {
	  for (int i=0;i<=elem;i++)
	    {
	      QString tagname("maxdatagap_"+QString().setNum(i));
	      if(i<=box) {
		if (element.firstChildElement(tagname).text().toInt()
		    !=dataGapBoxes[i]->value()) {
		  emit changeDom(element, tagname, 
				 QString().setNum(dataGapBoxes[i]->value())); }}
	      else {
		emit removeDom(element, tagname);
	      }
	    }
	}
	else {
	  for (int i=0;i<=box;i++)
	    {
	      QString tagname("maxdatagap_"+QString().setNum(i));
	      if(i<=elem) {
		if (element.firstChildElement(tagname).text().toInt()
		    !=dataGapBoxes[i]->value()) {
		  emit changeDom(element, tagname, 
				 QString().setNum(dataGapBoxes[i]->value())); }}
	      else {
		emit addDom(element, tagname, 
			    QString().setNum(dataGapBoxes[i]->value()));
	      }
	    }
	}
      }
      else 
	{
	  int elem = element.firstChildElement("maxwavenumber").text().toInt();
	  for(int i=0;i<=elem; i++)
	    {
	      QString tagname("maxdatagap_"+QString().setNum(i));
	      if(element.firstChildElement(tagname).text().toFloat()
		 != dataGapBoxes[i]->value()) {
		emit changeDom(element, tagname, 
			       QString().setNum(dataGapBoxes[i]->value())); }
	    }
	}
      if(element.firstChildElement("ringwidth").text().toDouble()
	 !=ringBox->value()) {
	emit changeDom(element, QString("ringwidth"), 
		       QString().setNum(ringBox->value()));
      }
      if(element.firstChildElement("influenceradius").text().toDouble()
	 !=influenceBox->value()) {
	emit changeDom(element, QString("influenceradius"), 
		       QString().setNum(influenceBox->value()));
      }
      if(element.firstChildElement("convergence").text().toDouble()
	 !=convergenceBox->value()) {
	emit changeDom(element, QString("convergence"), 
		       QString().setNum(convergenceBox->value()));
      }
      if(element.firstChildElement("maxiterations").text().toInt()
	 !=iterations->value()) {
	emit changeDom(element, QString("maxiterations"), 
		       QString().setNum(iterations->value()));
      }
      if(element.firstChildElement("boxdiameter").text().toDouble()
	 !=diameterBox->value()) {
	emit changeDom(element, QString("boxdiameter"), 
		       QString().setNum(diameterBox->value()));
      }
      if(element.firstChildElement("numpoints").text().toDouble()
	 !=numPointsBox->value()) {
	emit changeDom(element, QString("numpoints"), 
		       QString().setNum(numPointsBox->value()));
      }
    }
  setPanelChanged(false);
  return true;
}

VTDPanel::VTDPanel()
{
  QLabel *dirLabel = new QLabel(tr("VTD Output Directory"));
  dir = new QLineEdit();
  browse = new QPushButton(tr("Browse.."));
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
  QLabel *bottomLevel = new QLabel(tr("Bottom Level"));
  bLBox = new QSpinBox;
  QLabel *topLevel = new QLabel(tr("Top Level"));
  tLBox = new QSpinBox;
  QLabel *innerRad = new QLabel(tr("Inner Radius"));
  iRBox = new QSpinBox;
  QLabel *outerRad = new QLabel(tr("Outer Radius"));
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
  maxWaveNumBox = new QSpinBox;
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
      dir->clear();
      dir->insert(parameter); }
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
      if(parameter.toInt() != dataGapBoxes.count()-1)
	{
	  createDataGaps();
	}
      for(int i = 0; i <dataGapBoxes.count(); i++)
	{
	  QString tagname("maxdatagap_"+QString().setNum(i));
	  double dataGap = panelElement.firstChildElement(tagname).text().toDouble();
	  dataGapBoxes[i]->setValue(dataGap);
	}
    }

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
      if(element.firstChildElement("dir").text()!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
      }
      if(element.firstChildElement("geometry").text()
	 !=geometryOptions->value(geometryBox->currentText())) {
	emit changeDom(element, QString("geometry"), 
		       geometryOptions->value(geometryBox->currentText()));
      }
      if(element.firstChildElement("closure").text()
	 !=closureOptions->value(closureBox->currentText()))
	{
	  emit changeDom(element, QString("closure"), 
			 closureOptions->value(closureBox->currentText()));
	}
      if(element.firstChildElement("reflectivity").text()
	 !=reflectivityOptions->value(refBox->currentText())) {
	emit changeDom(element, QString("reflectivity"), 
		       reflectivityOptions->value(refBox->currentText()));
      }
      if(element.firstChildElement("velocity").text()
	 !=velocityOptions->value(velBox->currentText())) {
	emit changeDom(element, QString("velocity"), 
		       velocityOptions->value(velBox->currentText()));
      }
      if(element.firstChildElement("bottomlevel").text().toInt()
	 !=bLBox->value()) {
	emit changeDom(element, QString("bottomlevel"), 
		       QString().setNum(bLBox->value()));
      }
      if(element.firstChildElement("toplevel").text().toInt()
	 !=tLBox->value()) {
	emit changeDom(element, QString("toplevel"), 
		       QString().setNum(tLBox->value()));
      }
      if(element.firstChildElement("innerradius").text().toInt()
	 !=iRBox->value()) {
	emit changeDom(element, QString("innerradius"), 
		       QString().setNum(iRBox->value()));
      }
      if(element.firstChildElement("outerradius").text().toInt()
	 !=oRBox->value()) {
	emit changeDom(element, QString("outerradius"), 
		       QString().setNum(oRBox->value()));
      }
      if(element.firstChildElement("maxwavenumber").text().toInt()
	 !=maxWaveNumBox->value()) {
	
	int box = maxWaveNumBox->value();
	int elem = element.firstChildElement("maxwavenumber").text().toInt();
	
	emit changeDom(element, QString("maxwavenumber"), 
		       QString().setNum(maxWaveNumBox->value()));
	
	if (box<elem) {
	  for (int i=0;i<=elem;i++)
	    {
	      QString tagname("maxdatagap_"+QString().setNum(i));
	      if(i<=box) {
		if (element.firstChildElement(tagname).text().toInt()
		    !=dataGapBoxes[i]->value()) {
		  emit changeDom(element, tagname, 
				 QString().setNum(dataGapBoxes[i]->value())); }}
	      else {
		emit removeDom(element, tagname);
	      }
	    }
	}
	else {
	  for (int i=0;i<=box;i++)
	    {
	      QString tagname("maxdatagap_"+QString().setNum(i));
	      if(i<=elem) {
		if (element.firstChildElement(tagname).text().toInt()
		    !=dataGapBoxes[i]->value()) {
		  emit changeDom(element, tagname, 
				 QString().setNum(dataGapBoxes[i]->value())); }}
	      else {
		emit addDom(element, tagname, 
			    QString().setNum(dataGapBoxes[i]->value()));
	      }
	    }
	}
      }
      else 
	{
	  int elem = element.firstChildElement("maxwavenumber").text().toInt();
	  for(int i=0;i<=elem; i++)
	    {
	      QString tagname("maxdatagap_"+QString().setNum(i));
	      if(element.firstChildElement(tagname).text().toFloat()
		 != dataGapBoxes[i]->value()) {
		emit changeDom(element, tagname, 
			       QString().setNum(dataGapBoxes[i]->value())); }
	    }
	}
      if(element.firstChildElement("ringwidth").text().toDouble()
	 !=ringBox->value()) {
	emit changeDom(element, QString("ringwidth"), 
		       QString().setNum(ringBox->value()));
      }
    }
  setPanelChanged(false);
  return true;
}

HVVPPanel::HVVPPanel()
{
  //Do nothing
  setPanelChanged(false);
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
  dir = new QLineEdit();
  browse = new QPushButton(tr("Browse.."));
  connect(browse, SIGNAL(clicked()), this, SLOT(getDirectory()));
  QGridLayout *dirLayout = new QGridLayout;
  dirLayout->addWidget(dirLabel, 0, 0);
  dirLayout->addWidget(dir, 1, 0, 1, 3);
  dirLayout->addWidget(browse, 1,3);
  QVBoxLayout *main = new QVBoxLayout;
  main->addLayout(dirLayout);
  main->addStretch(1);
  setLayout(main);

  connect(dir, SIGNAL(textChanged(const QString&)),
	  this, SLOT(valueChanged(const QString&)));

  setPanelChanged(false);
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
      dir->clear();
      dir->insert(parameter); }
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
      if(element.firstChildElement("dir").text()!=dir->text()) {
	emit changeDom(element, QString("dir"), dir->text());
      }
    }
  setPanelChanged(false);
  return true;
}

GraphicsPanel::GraphicsPanel()
{

  graphParameters = new QGroupBox(tr("Parameters for Graph Display"));
  graphParameters->setCheckable(true);
  QGridLayout *graph = new QGridLayout;
  
  QLabel *pMax = new QLabel(tr("Maximum Pressure"));
  pMaxBox = new QDoubleSpinBox;
  pMaxBox->setRange(0,2000);
  pMaxBox->setDecimals(1);
  
  QLabel *pMin = new QLabel(tr("Minimum Pressure"));
  pMinBox = new QDoubleSpinBox;
  pMinBox->setRange(0, 2000);
  pMinBox->setDecimals(1);
  
  QLabel *rmwMax = new QLabel(tr("Maximum RMW"));
  rmwMaxBox = new QDoubleSpinBox;
  rmwMaxBox->setDecimals(1);
  
  QLabel *rmwMin = new QLabel(tr("Minimum RMW"));
  rmwMinBox = new QDoubleSpinBox;
  rmwMinBox->setDecimals(1);
  
  QLabel *beginTimeLabel = new QLabel(tr("Beginning Time"));
  beginTime = new QDateTimeEdit();
  
  QLabel *endTimeLabel = new QLabel (tr("Endding Time"));
  endTime = new QDateTimeEdit();
  
  graph->addWidget(pMax, 0,0);
  graph->addWidget(pMaxBox, 0, 1);
  graph->addWidget(pMin, 1, 0);
  graph->addWidget(pMinBox, 1, 1);
  graph->addWidget(rmwMax, 0, 2);
  graph->addWidget(rmwMaxBox, 0, 3);
  graph->addWidget(rmwMin, 1, 2);
  graph->addWidget(rmwMinBox, 1, 3);
  graph->addWidget(beginTimeLabel, 2, 0);
  graph->addWidget(beginTime, 2,1);
  graph->addWidget(endTimeLabel, 3, 0);
  graph->addWidget(endTime, 3,1);
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
  connect(beginTime, SIGNAL(dateTimeChanged(const QDateTime&)),
	  this, SLOT(valueChanged(const QDateTime&)));
  connect(endTime, SIGNAL(dateTimeChanged(const QDateTime&)), 
	  this, SLOT(valueChanged(const QDateTime&)));

  setPanelChanged(false);
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
      }
    }
  setPanelChanged(false);
  return true;
}

QCPanel::QCPanel()
{
  QGroupBox *qcParameters = new QGroupBox(tr("Quality Control Parameters"));
  
  QLabel *velThresLabel = new QLabel(tr("Ignore Velocities With Magnitude Less Than:"));
  velocityThreshold = new QDoubleSpinBox;
  velocityThreshold->setDecimals(3);
  velocityThreshold->setRange(0,10);
  velocityThreshold->setValue(1);
 
  QLabel *specThresLabel = new QLabel(tr("Ignore Gates With Spectral Width Greater Than:"));
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
  paramLayout->addWidget(velThresLabel,0,0,1,3);
  paramLayout->addWidget(velocityThreshold,0,2,1,1);
  paramLayout->addWidget(specThresLabel,1,0,1,3);
  paramLayout->addWidget(spectralThreshold,1,2,1,1);
  paramLayout->addWidget(bbLabel,2,0,1,3);
  paramLayout->addWidget(bbSegmentSize,2,2,1,1);
  paramLayout->addWidget(maxFoldLabel,3,0,1,3);
  paramLayout->addWidget(maxFoldCount,3,2,1,1);
  qcParameters->setLayout(paramLayout);

  QGroupBox *findWind = new QGroupBox(tr("Method For Finding Reference Wind"));
  vad = new QRadioButton(tr("Use GVAD and VAD Algorithms"), findWind);
  user = new QRadioButton(tr("Enter Environmental Wind Parameters"), findWind);
  known = new QRadioButton(tr("Use Available AWEPS Data"), findWind);

  QFrame *vadParameters = new QFrame;

  useGVAD = new QRadioButton(tr("Use GVAD Algorithm to Find Mean Winds"),
			     vadParameters);

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

  QVBoxLayout *vadLayout = new QVBoxLayout;
  vadLayout->addWidget(useGVAD);
  vadLayout->addLayout(vadLevelsLayout);
  vadLayout->addLayout(numCoLayout);
  vadParameters->setLayout(vadLayout);
  vadParameters->hide();

  QFrame *userParameters = new QFrame;
 
  QLabel *windSpeedLabel = new QLabel(tr("Wind Speed"));
  windSpeed = new QDoubleSpinBox;
  windSpeed->setDecimals(2);
  windSpeed->setRange(0, 200);
  QHBoxLayout *windSpeedLayout = new QHBoxLayout;
  windSpeedLayout->addSpacing(20);
  windSpeedLayout->addWidget(windSpeedLabel);
  windSpeedLayout->addWidget(windSpeed);

  QLabel *windDirectionLabel = new QLabel(tr("Wind Direction"));
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
  QLabel *knownDirLabel = new QLabel(tr("AWEPS Data Directory"));
  dir = new QLineEdit();
  browse = new QPushButton("Browse..");
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
  connect(velocityThreshold, SIGNAL(valueChanged(const QString&)),
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
    if(name == "vel_threshold") {
      velocityThreshold->setValue(parameter.toDouble()); }
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
    child = child.nextSiblingElement();
  }
  setPanelChanged(false);
}

bool QCPanel::updateConfig()
{
  QDomElement element = getPanelElement();
  if (checkPanelChanged()) 
    {
      if(element.firstChildElement("vel_threshold").text().toDouble()
	!=velocityThreshold->value()) {
	emit changeDom(element, QString("vel_threshold"), 
		       QString().setNum(velocityThreshold->value()));
      }
      if(element.firstChildElement("sw_threshold").text().toDouble()
	 !=spectralThreshold->value()) {
	emit changeDom(element, QString("sw_threshold"),
		       QString().setNum(spectralThreshold->value()));
      }
      if(element.firstChildElement("bbcount").text().toInt()
	 !=bbSegmentSize->value()) {
	emit changeDom(element,QString("bbcount"),
		       QString().setNum(bbSegmentSize->value()));
      }
      if(element.firstChildElement("maxfold").text().toInt()
	 !=maxFoldCount->value()) {
	emit changeDom(element, QString("maxfold"),
		       QString().setNum(maxFoldCount->value()));
      }
      if(vad->isChecked()) {
	emit changeDom(element, QString("wind_method"), QString("vad"));
	
	if(element.firstChildElement("vadlevels").text().toInt()
	   != vadLevels->value()) {
	  emit changeDom(element, QString("vadlevels"),
			  QString().setNum(vadLevels->value()));
	}
	if(element.firstChildElement("numcoeff").text().toInt()
	   != numCoefficients->value()) {
	  emit changeDom(element, QString("numcoeff"),
			 QString().setNum(numCoefficients->value()));
	}
      }
      if(user->isChecked()) {
	emit changeDom(element, QString("wind_method"), QString("user"));
	
	if(element.firstChildElement("windspeed").text().toDouble()
	   != windSpeed->value()) {
	  emit changeDom(element, QString("windspeed"),
			  QString().setNum(windSpeed->value()));
	}
	if(element.firstChildElement("winddirection").text().toDouble()
	   != windDirection->value()) {
	  emit changeDom(element, QString("winddirection"),
			 QString().setNum(windDirection->value()));
	}
      }
      if(known->isChecked()) {
	emit changeDom(element, QString("wind_method"), QString("known"));
	
	if(element.firstChildElement("awips_dir").text()
	   != dir->text()) {
	  emit changeDom(element, QString("awips_dir"),dir->text());
	}
      }
      setPanelChanged(false);
    }
  return true;
}

/*
  void RadarPanel::fillRadAubryarHash()
  {

  
    Was used before the xml file for radar information was created
    This is kept on hand for now incase the info in the xml is overwritten
    
    radarNameOptions = new QHash<QString, QString>;
    radarLocations = new QHash<QString, QPointF>;
    radarNameOptions->insert(QString("Please select a radar"), QString(""));
    radarLocations->insert(QString("Please select a radar"), QPointF(0.0,0.0));
    radarNameOptions->insert(QString("RCWF in Taiwan"),QString("RCWF"));
   radarLocations->insert(QString("RCWF in Taiwan"), QPointF(121.773, 25.073));
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
    radarLocations->insFrom:	
From:	Lisa Mauger <lmauger@mines.edu>  changeert(QString("KBMX in Birmingham, AL"),
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
