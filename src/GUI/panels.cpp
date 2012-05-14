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
#include "DataObjects/SimplexData.h"
#include "DataObjects/VortexData.h"
#include "DataObjects/GriddedData.h"
#include <math.h>

#include <QtGui>
#include <QFileDialog>

VortexPanel::VortexPanel():AbstractPanel()
{
    QLabel *vortexNameLabel = new QLabel(tr("Vortex Name:"));
    vortexName = new QLineEdit();
    QHBoxLayout *name = new QHBoxLayout;
    name->addWidget(vortexNameLabel);
    name->addWidget(vortexName);

    QLabel *latLabel = new QLabel(tr("Vortex Latitude:"));
    latBox = new QDoubleSpinBox();
    latBox->setRange(-90,90);
    latBox->setDecimals(2);
    QHBoxLayout *lat = new QHBoxLayout;
    lat->addWidget(latLabel);
    lat->addStretch();
    lat->addWidget(latBox);

    QLabel *longLabel = new QLabel(tr("Vortex Longitude"));
    longBox = new QDoubleSpinBox();
    longBox->setRange(-180, 180);
    longBox->setDecimals(2);
    QHBoxLayout *longitude = new QHBoxLayout;
    longitude->addWidget(longLabel);
    longitude->addStretch();
    longitude->addWidget(longBox);

    QLabel *directionLabel = new QLabel(tr("Direction of Vortex Movement (Degrees CW from North)"));
    directionBox = new QDoubleSpinBox();
    directionBox->setRange(0, 360);
    directionBox->setDecimals(2);
    QHBoxLayout *direction = new QHBoxLayout;
    direction->addWidget(directionLabel);
    direction->addStretch();
    direction->addWidget(directionBox);

    QLabel *speedLabel = new QLabel(tr("Speed of Vortex Movement (m/s)"));
    speedBox = new QDoubleSpinBox();
    speedBox->setRange(0,100);
    speedBox->setDecimals(2);
    QHBoxLayout *speed = new QHBoxLayout;
    speed->addWidget(speedLabel);
    speed->addStretch();
    speed->addWidget(speedBox);

    QLabel *rmwLabel = new QLabel(tr("Radius of Maximum Wind Estimate (km)"));
    rmwBox = new QDoubleSpinBox();
    rmwBox->setRange(0,100);
    rmwBox->setDecimals(2);
    QHBoxLayout *rmwLayout = new QHBoxLayout;
    rmwLayout->addWidget(rmwLabel);
    rmwLayout->addStretch();
    rmwLayout->addWidget(rmwBox);

    QLabel *obsLabel = new QLabel(tr("Time of Above Observations"));
    obsDateTime = new QDateTimeEdit();
    obsDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
    QDate minDate(QDate::fromString(QString("1900-01-01"),"yyyy-MM-dd"));
    QTime minTime(QTime::fromString(QString("00:00:00"), "hh:mm:ss"));
    obsDateTime->setMinimumTime(minTime);
    obsDateTime->setMinimumDate(minDate);
    obsDateTime->setDateTime(QDateTime::currentDateTime());
    QHBoxLayout *obs = new QHBoxLayout;
    obs->addWidget(obsLabel);
    obs->addStretch();
    obs->addWidget(obsDateTime);

    QLabel *workingDirLabel = new QLabel(tr("Working Directory"));
    QGridLayout *dirLayout = new QGridLayout();
    dirLayout->addWidget(workingDirLabel, 0, 0);
    dirLayout->addWidget(dir, 1, 0, 1, 3);
    dirLayout->addWidget(browse, 1, 3);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addLayout(name);
    //layout->addWidget(stormType);
    layout->addLayout(lat);
    layout->addLayout(longitude);
    layout->addLayout(direction);
    layout->addLayout(speed);
    layout->addLayout(obs);
    layout->addLayout(rmwLayout);
    layout->addLayout(dirLayout);
    layout->addStretch(1);
    setLayout(layout);

    // Add the widgets that should not be adjustable at runtime to
    // the turnOffWhenRunning QList
    turnOffWhenRunning.append(dir);
    turnOffWhenRunning.append(workingDirLabel);
    turnOffWhenRunning.append(browse);

    connect(vortexName, SIGNAL(textChanged(const QString&)), this, SLOT(valueChanged()));
    connect(latBox, SIGNAL(valueChanged(const QString&)), this, SLOT(valueChanged()));
    connect(longBox, SIGNAL(valueChanged(const QString&)), this, SLOT(valueChanged()));
    connect(directionBox, SIGNAL(valueChanged(const QString&)), this, SLOT(valueChanged()));
    connect(speedBox, SIGNAL(valueChanged(const QString&)), this, SLOT(valueChanged()));
    connect(rmwBox, SIGNAL(valueChanged(const QString&)), this, SLOT(valueChanged()));
    connect(dir, SIGNAL(textChanged(const QString&)), this, SLOT(valueChanged()));
    connect(obsDateTime, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(valueChanged()));
    connect(this, SIGNAL(workingDirectoryChanged()), this, SLOT(createDirectory()));

    setPanelChanged(false);
}

VortexPanel::~VortexPanel()
{
    turnOffWhenRunning.clear();
    delete vortexName;
    delete latBox;
    delete longBox;
    delete directionBox;
    delete speedBox;
    delete rmwBox;
    delete obsDateTime;
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
            speedBox->setValue(parameter.toDouble()); }
        if(name == "rmw") {
            rmwBox->setValue(parameter.toDouble()); }
        if(name == "obsdate") {
            obsDateTime->setDate(QDate::fromString(parameter, "yyyy-MM-dd")); }
        if(name == "obstime") {
            obsDateTime->setTime(QTime::fromString(parameter, "hh:mm:ss")); }
        if(name == "dir")  {
            if(parameter!=QString("default")) {
                dir->clear();
                dir->insert(parameter);
                emit workingDirectoryChanged();
            }
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
        if(fabs(getFromElement("lat").toDouble()-latBox->value())>=0.01) {
            emit changeDom(element, QString("lat"),
                           QString().setNum(latBox->value()));
        }
        if(fabs(getFromElement("lon").toDouble()-longBox->value())>=0.01) {
            emit changeDom(element, QString("lon"),
                           QString().setNum(longBox->value()));
        }
        if(fabs(getFromElement("direction").toDouble()-directionBox->value())
                >=0.01) {
            emit changeDom(element, QString("direction"),
                           QString().setNum(directionBox->value()));
        }
        if(fabs(getFromElement("speed").toDouble()-speedBox->value())>=0.01) {
            emit changeDom(element, QString("speed"),
                           QString().setNum(speedBox->value()));
        }
        if(fabs(getFromElement("rmw").toDouble()-rmwBox->value())>=0.01) {
            emit changeDom(element, QString("rmw"),
                           QString().setNum(rmwBox->value()));
            emit rmwChanged();
        }
        if(getFromElement("dir")!=dir->text()) {
            emit changeDom(element, QString("dir"), dir->text());
            emit workingDirectoryChanged();
        }
        if(getFromElement("obsdate")
                !=obsDateTime->date().toString("yyyy-MM-dd")) {
            emit changeDom(element, QString("obsdate"),
                           obsDateTime->date().toString("yyyy-MM-dd"));
        }
        if(getFromElement("obstime")
                !=obsDateTime->time().toString("hh:mm:ss")) {
            emit changeDom(element, QString("obstime"),
                           obsDateTime->time().toString("hh:mm:ss"));
        }
    }
    setPanelChanged(false);
    return true;
}

bool VortexPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...
    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}



RadarPanel::RadarPanel():AbstractPanel()
{
    QLabel *radarNameLabel = new QLabel(tr("Radar Name:"));
    radarName = new QComboBox();
    QString resources = QCoreApplication::applicationDirPath() + "/../Resources";
    QDir resourceDir = QDir(resources);
    radars = new Configuration(this,resourceDir.filePath(QString("vortrac_radarList.xml")));
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

    QLabel *latLabel = new QLabel(tr("Radar Latitude (deg):"));
    //radarLatBox = new QDoubleSpinBox;
    radarLatBox->setRange(-90,90);
    radarLatBox->setDecimals(3);
    QHBoxLayout *lat = new QHBoxLayout;
    lat->addWidget(latLabel);
    lat->addWidget(radarLatBox);

    QLabel *longLabel = new QLabel(tr("Radar Longitude (deg):"));
    //radarLongBox = new QDoubleSpinBox();
    radarLongBox->setDecimals(3);
    radarLongBox->setRange(-180, 180);
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
    //radarFormatOptions->insert(QString("Analytic Model"), QString("MODEL"));
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
    QDate minDate(QDate::fromString(QString("1900-01-01"),"yyyy-MM-dd"));
    QTime minTime(QTime::fromString(QString("00:00:00"), "hh:mm:ss"));
    startDateTime->setMinimumTime(minTime);
    startDateTime->setMinimumDate(minDate);
    startDateTime->setDateTime(QDateTime::currentDateTime());
    QHBoxLayout *startLayout = new QHBoxLayout;
    startLayout->addWidget(start);
    startLayout->addWidget(startDateTime);

    QLabel *end = new QLabel(tr("End Date and Time"));
    endDateTime = new QDateTimeEdit();
    endDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
    endDateTime->setMinimumTime(minTime);
    endDateTime->setMinimumDate(minDate);
    endDateTime->setDateTime(QDateTime::currentDateTime().addDays(3));
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

    // Add the widgets that should not be adjustable at runtime to
    // the turnOffWhenRunning QList
    turnOffWhenRunning.append(dir);
    turnOffWhenRunning.append(browse);
    turnOffWhenRunning.append(radarDirLabel);
    turnOffWhenRunning.append(radarFormat);
    turnOffWhenRunning.append(radarFormatLabel);
    turnOffWhenRunning.append(radarName);
    turnOffWhenRunning.append(radarNameLabel);

    connect(radarName, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(radarLatBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(radarLongBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(dir, SIGNAL(textChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(radarFormat, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(startDateTime, SIGNAL(dateTimeChanged(const QDateTime&)),
            this, SLOT(valueChanged()));
    connect(endDateTime, SIGNAL(dateTimeChanged(const QDateTime&)),
            this, SLOT(valueChanged()));
    connect(radarFormat, SIGNAL(activated(const QString&)),
            this, SLOT(checkForAnalytic(const QString&)));

    connect(radarName, SIGNAL(activated(const QString&)),
            this, SLOT(radarChanged(const QString&)));
    setPanelChanged(false);

}

RadarPanel::~RadarPanel()
{
    turnOffWhenRunning.clear();
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
                emit workingDirectoryChanged();
            }
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
        if(getFromElement("name")!= radarName->currentText().left(4)) {
            if(radarName->currentText().left(4)!=QString("Plea")) {
                emit changeDom(element, "name",
                               radarName->currentText().left(4));
            }
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
    if(checkDates())
        return true;
    else
        return false;
}

bool RadarPanel::checkDates()
{
    //Message::toScreen("In RadarPanel CheckDates");
    if(startDateTime->dateTime() >= endDateTime->dateTime()) {
        QString message("start date and time must occur before end date and time");
        emit log(Message(message, 0, this->objectName(),Red));
        return false;
    }
    return true;
}

bool RadarPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...

    QString temp(radarName->currentText().left(4));
    int index = radarName->findText(temp, Qt::MatchStartsWith);
    if (index <= 0) {
        emit log(Message(QString(),0,this->objectName(),Red,
                         QString(tr("Please select a radar"))));
        return false;
    }


    temp = QString(radarFormat->currentText());
    if(radarFormatOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a radar file type"))));
        return false;
    }

    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}

CappiPanel::CappiPanel():AbstractPanel()
{
    QLabel *cappiDirLabel = new QLabel(tr("CAPPI Output Directory"));
    QGridLayout *cappiDir = new QGridLayout;
    cappiDir->addWidget(cappiDirLabel, 0, 0);
    cappiDir->addWidget(dir, 1, 0, 1, 3);
    cappiDir->addWidget(browse, 1,3);

    QGroupBox *grid = new QGroupBox(tr("Griding Configurations"));

    QLabel *xdim = new QLabel(tr("Grid Dimension in X Direction"));
    QLabel *ydim = new QLabel(tr("Grid Dimension in Y Direction"));
    QLabel *zdim = new QLabel(tr("Grid Dimension in Z Direction"));
    xDimBox = new QDoubleSpinBox;
    xDimBox->setDecimals(0);
    xDimBox->setRange(0,GriddedData::getMaxIDim());
    xDimBox->setValue(150);
    yDimBox = new QDoubleSpinBox;
    yDimBox->setDecimals(0);
    yDimBox->setRange(0,GriddedData::getMaxJDim());
    yDimBox->setValue(150);
    zDimBox = new QDoubleSpinBox;
    zDimBox->setDecimals(0);
    zDimBox->setRange(1,GriddedData::getMaxKDim());
    zDimBox->setValue(3);

    QLabel *xGrid = new QLabel(tr("X Grid Spacing (km)"));
    xGridBox = new QDoubleSpinBox;
    xGridBox->setDecimals(1);
    xGridBox->setRange(1,1);
    xGridBox->setValue(1);

    QLabel *yGrid = new QLabel(tr("Y Grid Spacing (km)"));
    yGridBox = new QDoubleSpinBox;
    yGridBox->setDecimals(1);
    yGridBox->setRange(1,1);
    yGridBox->setValue(1);

    QLabel *zGrid = new QLabel(tr("Z Grid Spacing (km)"));
    zGridBox = new QDoubleSpinBox;
    zGridBox->setDecimals(1);
    zGridBox->setRange(1,1);
    zGridBox->setValue(1);

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

    /*
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
  */

    QLabel *interpolation = new QLabel(tr("Interpolation"));
    interpolationMethod = new QHash<QString, QString>;
    interpolationMethod->insert(QString("Cressman Interpolation"),
                                QString("cressman"));
    //interpolationMethod->insert(QString("Barnes Interpolation"),
    //			      QString("barnes"));
    //interpolationMethod->insert(QString("Closest Point Interpolation"),
    //QString("closestpoint"));
    //interpolationMethod->insert(QString("Bilinear Interpolation"),
    // QString("bilinear"));
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
    //layout->addLayout(adv);
    layout->addLayout(interpolationLayout);
    layout->addStretch(1);
    setLayout(layout);

    // Add the widgets that should not be adjustable at runtime to
    // the turnOffWhenRunning QList
    turnOffWhenRunning.append(dir);
    turnOffWhenRunning.append(browse);
    turnOffWhenRunning.append(cappiDirLabel);

    connect(dir, SIGNAL(textChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(xDimBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(yDimBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(zDimBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(xGridBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(yGridBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(zGridBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    /*
  connect(advUWindBox, SIGNAL(valueChanged(const QString&)),
   this, SLOT(valueChanged()));
  connect(advVWindBox, SIGNAL(valueChanged(const QString&)),
   this, SLOT(valueChanged()));
  */
    connect(intBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(this, SIGNAL(workingDirectoryChanged()),
            this, SLOT(createDirectory()));

    // These connections allow individual widgets to notify the panel when
    // a parameter has changed.

    setPanelChanged(false);
}

CappiPanel::~CappiPanel()
{
    turnOffWhenRunning.clear();
    delete xDimBox;
    delete yDimBox;
    delete zDimBox;
    delete xGridBox;
    delete yGridBox;
    delete zGridBox;
    /*
    delete advUWindBox;
    delete advVWindBox;
  */
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
                emit workingDirectoryChanged();
            }
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
        /*
    if (name == "adv_u") {
      advUWindBox->setValue(parameter.toDouble()); }
    if( name == "adv_v") {
      advVWindBox->setValue(parameter.toDouble()); }
    */
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
        if(fabs(getFromElement("xdim").toDouble()-xDimBox->value())>=0.1) {
            emit changeDom(element, QString("xdim"),
                           QString().setNum(xDimBox->value()));
        }
        if(fabs(getFromElement("ydim").toDouble()-yDimBox->value())>=0.1) {
            emit changeDom(element, QString("ydim"),
                           QString().setNum(yDimBox->value()));
        }
        if(fabs(getFromElement("zdim").toDouble()-zDimBox->value())>=0.1) {
            emit changeDom(element, QString("zdim"),
                           QString().setNum(zDimBox->value()));
        }
        if(fabs(getFromElement("xgridsp").toDouble()-xGridBox->value())>=0.1) {
            emit changeDom(element, QString("xgridsp"),
                           QString().setNum(xGridBox->value()));
        }
        if(fabs(getFromElement("ygridsp").toDouble()-yGridBox->value())>=0.1) {
            emit changeDom(element, QString("ygridsp"),
                           QString().setNum(yGridBox->value()));
        }
        if(fabs(getFromElement("zgridsp").toDouble()-zGridBox->value())>=0.1) {
            emit changeDom(element, QString("zgridsp"),
                           QString().setNum(zGridBox->value()));
        }
        /*
      if(fabs(getFromElement("adv_u").toDouble()-advUWindBox->value())>=0.1) {
 emit changeDom(element, QString("adv_u"),
         QString().setNum(advUWindBox->value()));
      }
      if(fabs(getFromElement("adv_v").toDouble()-advVWindBox->value())>=0.1) {
 emit changeDom(element, QString("adv_v"),
         QString().setNum(advVWindBox->value()));
      }
      */
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
    //Message::toScreen("CappiPanel - Set default directory");
    //emit log(Message("CappiPanel - Set default directory"));
    if(!newDir->isAbsolute())
        newDir->makeAbsolute();
    if(!newDir->exists())
        newDir->mkpath(newDir->path());
    QString subDirectory("cappi");
    if(newDir->exists(subDirectory))
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                // NewDir+/cappi/ is used as working directory
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                // NewDir is used as working directory
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            // NewDir is used as working directory
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }

    // Otherwise make the subdirectory is it does not already exist
    if(newDir->mkpath(subDirectory)) {
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }
    }
    else {
        defaultDirectory->cd(newDir->path());
        delete newDir;
        return false;
    }

}

bool CappiPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...


    // Make sure that an interpolation has been selected
    QString temp(intBox->currentText());
    if(interpolationMethod->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a interpolation method in the configuration"))));
        return false;
    }

    // Gridded Data has a maximum number of points at (last I checked)
    // 256 in x
    // 256 in y
    // 20 in z
    // We must make sure that is how many we have selected

    // We should be using zgridsp here like in AnalysisThread 459

    if(xDimBox->value() > GriddedData::getMaxIDim()) {
        emit log(Message(QString(),0, this->objectName(), Red,
                         QString(tr("Cappi X dimension has exceeded ")+QString().setNum(GriddedData::getMaxIDim())+tr(" points, Please decrease the dimension of cappi in x"))));
        return false;
    }
    if(yDimBox->value() > GriddedData::getMaxJDim()) {
        emit log(Message(QString(),0, this->objectName(), Red,
                         QString(tr("Cappi Y dimension has exceeded ")+QString().setNum(GriddedData::getMaxJDim())+tr(" points, Please decrease the dimension of cappi in y"))));
        return false;
    }
    if(zDimBox->value() > GriddedData::getMaxKDim()) {
        emit log(Message(QString(),0, this->objectName(), Red,
                         QString(tr("Cappi Z dimension has exceeded ")+QString().setNum(GriddedData::getMaxIDim())+tr(" points, Please decrease the dimension of cappi in z"))));
        return false;
    }

    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}

CenterPanel::CenterPanel()
    :AbstractPanel()
{
    QLabel *dirLabel = new QLabel(tr("Center Output Directory"));
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
    //closureOptions->insert(QString("Original-HVVP"),
    //			 QString("original_hvvp"));

    // Add addtional options here

    closureOptions->insert(QString("Select closure assumption"),
                           QString(""));

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
    bLBox = new QDoubleSpinBox;
    bLBox->setRange(1,1);
    bLBox->setDecimals(2);
    bLBox->setValue(1);
    QLabel *topLevel = new QLabel(tr("Top Level (km)"));
    tLBox = new QDoubleSpinBox;
    tLBox->setRange(1,1);
    tLBox->setDecimals(2);
    tLBox->setValue(3);
    QLabel *innerRad = new QLabel(tr("Inner Radius (km)"));
    iRBox = new QDoubleSpinBox;
    iRBox->setDecimals(2);
    iRBox->setRange(1,120);
    iRBox->setValue(10);
    QLabel *outerRad = new QLabel(tr("Outer Radius (km)"));
    oRBox = new QDoubleSpinBox;
    oRBox->setDecimals(2);
    oRBox->setRange(2,150);
    oRBox->setValue(40);

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
    maxWaveNumBox->setRange(0, 4);
    maxWaveNumBox->setValue(0);
    QHBoxLayout *maxWaveLayout = new QHBoxLayout;
    maxWaveLayout->addWidget(maxWaveNum);
    maxWaveLayout->addWidget(maxWaveNumBox);
    createDataGaps();

    QLabel *searchCrit = new QLabel(tr("Center Finding Criteria"));
    criteriaOptions = new QHash<QString,QString>;
    criteriaOptions->insert(QString("Maximum Tangential Velocity"),
                            QString("MAXVT0"));
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
    ringBox->setRange(0.1,10);

    QLabel *influenceRadius = new QLabel(tr("Radius of Influence (km)"));
    influenceBox = new QDoubleSpinBox;
    influenceBox->setDecimals(1);
    influenceBox->setRange(2,10);
    influenceBox->setValue(4);

    QLabel *convergence = new QLabel(tr("Convergence Requirements"));
    convergenceBox = new QDoubleSpinBox;
    convergenceBox->setDecimals(3);
    convergenceBox->setRange(0.001,2);
    convergenceBox->setValue(0.05);

    QLabel *maxIterations = new QLabel(tr("Maximum Iterations for Process"));
    iterations = new QSpinBox;
    iterations->setRange(10,100);
    iterations->setValue(60);

    QLabel *boxDiameter = new QLabel(tr("Width of Search Zone"));
    diameterBox = new QDoubleSpinBox;
    diameterBox->setDecimals(1);
    diameterBox->setRange(9,25);
    diameterBox->setValue(12);

    QLabel *numPoints = new QLabel(tr("Number of Center Points"));
    numPointsBox = new QSpinBox;
    numPointsBox->setRange(1,25);
    numPointsBox->setValue(16);

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

    // Add the widgets that should not be adjustable at runtime to
    // the turnOffWhenRunning QList
    turnOffWhenRunning.append(dir);
    turnOffWhenRunning.append(dirLabel);
    turnOffWhenRunning.append(browse);

    connect(dir, SIGNAL(textChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(geometryBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(closureBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(refBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(velBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(critBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(bLBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(tLBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(iRBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(oRBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(maxWaveNumBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(numPointsBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(iterations, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(ringBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(influenceBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(convergenceBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(diameterBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(maxWaveNumBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(createDataGaps()));
    connect(this, SIGNAL(workingDirectoryChanged()),
            this, SLOT(createDirectory()));

    setPanelChanged(false);
}

CenterPanel::~CenterPanel()
{
    turnOffWhenRunning.clear();
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
                emit workingDirectoryChanged();
            }
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
        if(fabs(getFromElement("ringwidth").toDouble()-ringBox->value())>=0.1) {
            emit changeDom(element, QString("ringwidth"),
                           QString().setNum(ringBox->value()));
        }
        if(fabs(getFromElement("influenceradius").toDouble()-influenceBox->value())>=0.1) {
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
        if(fabs(getFromElement("boxdiameter").toDouble()-diameterBox->value())>=0.1) {
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
    //  Message::toScreen("CenterPanel - Set default directory");
    //  emit log(Message("CenterPanel - Set default directory"));
    QString subDirectory("center");
    if(!newDir->isAbsolute())
        newDir->makeAbsolute();
    if(newDir->exists(subDirectory))
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }
    if(newDir->mkdir(subDirectory)) {
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }
    }
    else {
        defaultDirectory->cd(newDir->path());
        delete newDir;
        return false;
    }
}

bool CenterPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...

    QString temp(geometryBox->currentText());
    if(geometryOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a geometry in the configuration"))));
        return false;
    }

    temp = closureBox->currentText();
    if(closureOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a closure assumption in the configuration"))));
        return false;
    }

    temp = refBox->currentText();
    if(reflectivityOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a reflectivity in the configuration"))));
        return false;
    }

    temp = velBox->currentText();
    if(velocityOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a velocity in the configuration"))));
        return false;
    }

    temp = critBox->currentText();
    if(criteriaOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a criteria in the configuration"))));
        return false;
    }

    // We have set minimums in the SimplexData containers that will
    // cause problems if we do not limit the number of levels, radii, and
    // centers used

    // Last I checked
    // SimplexData::maxLevels = 15
    // SimplexData::maxRadii = 30
    // SimplexData::maxCenters = 25
    // We get this directly now

    if((int)floor(tLBox->value()-bLBox->value()+1.5)>SimplexData::getMaxLevels()) {
        emit log(Message(QString(tr("Number of search levels in simplex exceeds the maximum of ")+QString().setNum(SimplexData::getMaxLevels())+tr(", Please decrease the number of levels")),0, this->objectName(), Red,
                         QString(tr("Too many simplex levels"))));
        return false;
    }

    if((int)floor(oRBox->value()-iRBox->value()+1.5)>SimplexData::getMaxRadii()) {
        emit log(Message(QString(tr("Number of search radii in simplex exceeds the maximum of ")+QString().setNum(SimplexData::getMaxRadii())+tr(", Please decrease the difference between inner and outer radii")),0, this->objectName(), Red,
                         QString(tr("Simplex outer-inner too large"))));
        return false;
    }

    if(numPointsBox->value()>SimplexData::getMaxCenters()) {
        emit log(Message(QString(),0, this->objectName(), Red,
                         QString(tr("Number initial centers in simplex exceeds the maximum of ")+QString().setNum(SimplexData::getMaxCenters())+tr(", Please decrease the number of initial centers"))));
        return false;
    }
    
    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}

ChooseCenterPanel::ChooseCenterPanel():AbstractPanel()
{
    QLabel *dirLabel = new QLabel(tr("ChooseCenter Output Directory"));
    QGridLayout *dirLayout = new QGridLayout;
    dirLayout->addWidget(dirLabel, 0, 0);
    dirLayout->addWidget(dir, 1, 0, 1, 3);
    dirLayout->addWidget(browse, 1,3);

    QLabel *start = new QLabel(tr("Start Date and Time"));
    startDateTime = new QDateTimeEdit();
    startDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
    QDate minDate(QDate::fromString(QString("1900-01-01"),"yyyy-MM-dd"));
    QTime minTime(QTime::fromString(QString("00:00:00"), "hh:mm:ss"));
    startDateTime->setMinimumTime(minTime);
    startDateTime->setMinimumDate(minDate);
    startDateTime->setDateTime(QDateTime::currentDateTime());
    QHBoxLayout *startLayout = new QHBoxLayout;
    startLayout->addWidget(start);
    startLayout->addWidget(startDateTime);

    QLabel *end = new QLabel(tr("End Date and Time"));
    endDateTime = new QDateTimeEdit();
    endDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
    endDateTime->setMinimumTime(minTime);
    endDateTime->setMinimumDate(minDate);
    endDateTime->setDateTime(QDateTime::currentDateTime().addDays(3));
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

    // Add the widgets that should not be adjustable at runtime to
    // the turnOffWhenRunning QList
    turnOffWhenRunning.append(dir);
    turnOffWhenRunning.append(dirLabel);
    turnOffWhenRunning.append(browse);

    connect(dir, SIGNAL(textChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(startDateTime, SIGNAL(dateTimeChanged(const QDateTime&)),
            this, SLOT(valueChanged()));
    connect(endDateTime, SIGNAL(dateTimeChanged(const QDateTime&)),
            this, SLOT(valueChanged()));
    connect(windWeightBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(stdDevWeightBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(ptsWeightBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(positionWeightBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(rmwWeightBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(velWeightBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(fTest95Button, SIGNAL(clicked(const bool)),
            this, SLOT(valueChanged()));
    connect(fTest95Button, SIGNAL(clicked(const bool)),
            this, SLOT(valueChanged()));
    connect(minVolumes, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(this, SIGNAL(workingDirectoryChanged()),
            this, SLOT(createDirectory()));

    setPanelChanged(false);
}

ChooseCenterPanel::~ChooseCenterPanel()
{
    turnOffWhenRunning.clear();
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
                emit workingDirectoryChanged();
            }
            else
                setPanelChanged(true);} // <---- this needs attention- NO GOOD? -LM
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
        if(fabs(getFromElement("wind_weight").toFloat()-windWeightBox->value())
                >=.01) {
            emit changeDom(element, QString("wind_weight"),
                           QString().setNum(windWeightBox->value()));
        }
        if(fabs(getFromElement("stddev_weight").toFloat()-stdDevWeightBox->value())>=0.01) {
            emit changeDom(element, QString("stddev_weight"),
                           QString().setNum(stdDevWeightBox->value()));
        }
        if(fabs(getFromElement("pts_weight").toFloat()-ptsWeightBox->value())
                >=0.01) {
            emit changeDom(element, QString("pts_weight"),
                           QString().setNum(ptsWeightBox->value()));
        }
        if(fabs(getFromElement("position_weight").toFloat()-positionWeightBox->value())>=0.01) {
            emit changeDom(element, QString("position_weight"),
                           QString().setNum(positionWeightBox->value()));
        }
        if(fabs(getFromElement("rmw_weight").toFloat()-rmwWeightBox->value())>=0.01) {
            emit changeDom(element, QString("rmw_weight"),
                           QString().setNum(rmwWeightBox->value()));
        }
        if(fabs(getFromElement("vt_weight").toFloat()-velWeightBox->value())>=0.01) {
            emit changeDom(element, QString("vt_weight"),
                           QString().setNum(velWeightBox->value()));
        }
        if(fTest99Button->isChecked()) {
            if(getFromElement("stats").toInt()!=99)
                emit changeDom(element, QString("stats"), QString().setNum(99));
        }
        if(fTest95Button->isChecked()) {
            if(getFromElement("stats").toInt()!=95)
                emit changeDom(element, QString("stats"), QString().setNum(95));
        }
    }
    setPanelChanged(false);
    if(checkDates())
        return true;
    else
        return false;
}

bool ChooseCenterPanel::setDefaultDirectory(QDir* newDir)
{
    //  Message::toScreen("ChooseCenterPanel - Set default directory");
    //  emit log(Message("ChooseCenterPanel - Set default directory"));
    QString subDirectory("choosecenter");
    if(!newDir->isAbsolute())
        newDir->makeAbsolute();
    if(newDir->exists(subDirectory))
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }
    if(newDir->mkdir(subDirectory)) {
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }
    }
    else {
        defaultDirectory->cd(newDir->path());
        delete newDir;
        return false;
    }

}

bool ChooseCenterPanel::checkDates()
{
    //  Message::toScreen("In ChooseCenterPanel CheckDates");
    if(startDateTime->dateTime() >= endDateTime->dateTime()) {
        QString message("start date and time must occur before end date and time");
        emit log(Message(message, 0, this->objectName(),Red));
        return false;
    }
    return true;
}

bool ChooseCenterPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...

    // The values of windWeightBox & stdWeightBox and ptsWeightBox should add
    // should add up to 1.00, if not we should give an error.

    float weightSum = windWeightBox->value()+stdDevWeightBox->value();
    weightSum += ptsWeightBox->value();
    if(fabs(weightSum-1.00) >= 0.01) {
        emit log(Message(QString("Values in Mean Weighting Scheme Must Add to 1.00"),0,this->objectName(),Red,QString("Mean Weight Values Incorrect")));
        return false;
    }

    // The values of positionWeightBox & rmwWeightBox & velWeightBox should
    // add up to 1.00, if not we should give an error.

    weightSum = positionWeightBox->value()+rmwWeightBox->value();
    weightSum += velWeightBox->value();
    if(fabs(weightSum-1.00) >= 0.01) {
        emit log(Message(QString("Values in Center Weighting Scheme Must Add to 1.00"), 0, this->objectName(), Red, QString("Center Weight Values Incorrect")));
        return false;
    }

    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}


VTDPanel::VTDPanel()
    :AbstractPanel()
{
    QLabel *dirLabel = new QLabel(tr("VTD Output Directory"));
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
    closureOptions->insert(QString("Original-HVVP"),
                           QString("original_hvvp"));

    // Add addtional options here

    closureOptions->insert(QString("Select Closure Assumption"),
                           QString(""));

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
    reflectivityOptions->insert(QString("Select Reflectivity"),
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

    QGroupBox *searchRegion = new QGroupBox(tr("VTD Analysis Region"));
    QGridLayout *search = new QGridLayout;
    QLabel *bottomLevel = new QLabel(tr("Bottom Level (km)"));
    bLBox = new QDoubleSpinBox;
    bLBox->setRange(1,1);
    bLBox->setDecimals(2);
    bLBox->setValue(1);
    QLabel *topLevel = new QLabel(tr("Top Level (km)"));
    tLBox = new QDoubleSpinBox;
    tLBox->setRange(1,1);
    tLBox->setDecimals(2);
    tLBox->setValue(3);
    QLabel *innerRad = new QLabel(tr("Inner Radius (km)"));
    iRBox = new QDoubleSpinBox;
    iRBox->setDecimals(2);
    iRBox->setRange(1,100);
    iRBox->setValue(3);
    QLabel *outerRad = new QLabel(tr("Outer Radius (km)"));
    oRBox = new QDoubleSpinBox;
    oRBox->setDecimals(2);
    oRBox->setRange(2,150);
    oRBox->setValue(80);
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
    maxWaveNumBox->setRange(0, 4);
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

    // Add the widgets that should not be adjustable at runtime to
    // the turnOffWhenRunning QList
    turnOffWhenRunning.append(dir);
    turnOffWhenRunning.append(dirLabel);
    turnOffWhenRunning.append(browse);


    connect(dir, SIGNAL(textChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(geometryBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(closureBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(refBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(velBox, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(bLBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(tLBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(iRBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(oRBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(maxWaveNumBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(ringBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(maxWaveNumBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(createDataGaps()));
    connect(this, SIGNAL(workingDirectoryChanged()),
            this, SLOT(createDirectory()));

    setPanelChanged(false);
}

VTDPanel::~VTDPanel()
{
    turnOffWhenRunning.clear();
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
                emit workingDirectoryChanged();
            }
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
        if(fabs(getFromElement("ringwidth").toDouble()-ringBox->value())>=0.1) {
            emit changeDom(element, QString("ringwidth"),
                           QString().setNum(ringBox->value()));
        }
    }
    setPanelChanged(false);
    return true;
}

bool VTDPanel::setDefaultDirectory(QDir* newDir)
{
    //  Message::toScreen("VTDPanel - Set default directory");
    //  emit log(Message("VTDPanel - Set default directory"));
    QString subDirectory("vtd");
    if(!newDir->isAbsolute())
        newDir->makeAbsolute();
    if(newDir->exists(subDirectory))
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }
    if(newDir->mkdir(subDirectory)) {
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }
    }
    else {
        defaultDirectory->cd(newDir->path());
        delete newDir;
        return false;
    }

}

bool VTDPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...

    QString temp(geometryBox->currentText());
    if(geometryOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a geometry in the configuration"))));
        return false;
    }

    temp = closureBox->currentText();
    if(closureOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a closure assumption in the configuration"))));
        return false;
    }

    temp = refBox->currentText();
    if(reflectivityOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a reflectivity in the configuration"))));
        return false;
    }

    temp = velBox->currentText();
    if(velocityOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a velocity in the configuration"))));
        return false;
    }

    // We have set minimums in the VortexData containers that will
    // cause problems if we do not limit the number of levels, radii, and
    // maxWaveNum

    // Last I checked
    // VortexData::maxLevels = 15
    // VortexData::maxRadii = 150
    // VortexData::maxWaveNum = 5
    // We get this directly now

    // Should be divided by gridspacing see AnalysisPage.cpp: 458ish

    if((int)floor((tLBox->value()-bLBox->value()) + 1.5) > VortexData::getMaxLevels()) {
        emit log(Message(QString(),0, this->objectName(), Red,
                         QString(tr("Number of search levels in vtd exceeds the maximum of ")+QString().setNum(VortexData::getMaxLevels())+tr(", Please decrease the number of search levels"))));
        return false;
    }

    if((int)floor((oRBox->value()-iRBox->value()) + 1.5) > VortexData::getMaxRadii()) {
        emit log(Message(QString(),0, this->objectName(), Red,
                         QString(tr("Number of search rings in vtd exceeds the maximum of ")+QString().setNum(VortexData::getMaxRadii())+tr(", Please decrease the number of search rings"))));
        return false;
    }

    if(maxWaveNumBox->value()+1 > VortexData::getMaxWaveNum()) {
        emit log(Message(QString(),0, this->objectName(), Red,
                         QString(tr("Maximum wave number in vtd exceeds the limit of ")+QString().setNum(VortexData::getMaxWaveNum())+tr(", Please decrease the number of wave numbers used"))));
        return false;
    }
    
    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}

HVVPPanel::HVVPPanel()
    :AbstractPanel()
{
    QGroupBox *hvvp = new QGroupBox(QString(tr("HVVP Parameters")));

    QLabel *numLevelsLabel = new QLabel(tr("Number of Levels Used in HVVP"));
    numLevels = new QSpinBox;
    numLevels->setRange(1,50);
    numLevels->setValue(14);
    QHBoxLayout *numLevelsLayout = new QHBoxLayout;
    numLevelsLayout->addWidget(numLevelsLabel);
    numLevelsLayout->addStretch();
    numLevelsLayout->addWidget(numLevels);

    QLabel *hgtStartLabel = new QLabel(tr("Height of the First Level (km)"));
    hgtStart = new QDoubleSpinBox;
    hgtStart->setDecimals(2);
    hgtStart->setRange(.5,50.0);
    hgtStart->setValue(.5);
    QHBoxLayout *hgtStartLayout = new QHBoxLayout;
    hgtStartLayout->addWidget(hgtStartLabel);
    hgtStartLayout->addStretch();
    hgtStartLayout->addWidget(hgtStart);

    QLabel *hIncLabel = new QLabel(tr("Thickness of the Levels (km)"));
    hInc = new QDoubleSpinBox;
    hInc->setDecimals(2);
    hInc->setRange(.01,2.0);
    hInc->setValue(.1);
    QHBoxLayout *hIncLayout = new QHBoxLayout;
    hIncLayout->addWidget(hIncLabel);
    hIncLayout->addStretch();
    hIncLayout->addWidget(hInc);

    QLabel *xtLabel = new QLabel(tr("Xt Multiplier for Thresholding"));
    xtBox = new QDoubleSpinBox();
    xtBox->setDecimals(1);
    xtBox->setRange(0,10);
    xtBox->setValue(2);
    QHBoxLayout *xtLayout = new QHBoxLayout;
    xtLayout->addWidget(xtLabel);
    xtLayout->addStretch();
    xtLayout->addWidget(xtBox);

    QVBoxLayout *hvvpLayout = new QVBoxLayout;
    hvvpLayout->addLayout(numLevelsLayout);
    hvvpLayout->addLayout(hgtStartLayout);
    hvvpLayout->addLayout(hIncLayout);
    hvvpLayout->addLayout(xtLayout);
    hvvp->setLayout(hvvpLayout);

    QVBoxLayout *main = new QVBoxLayout;
    main->addWidget(hvvp);
    main->addStretch();
    setLayout(main);

    connect(numLevels, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(hgtStart, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(hInc, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(xtBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));

    setPanelChanged(false);
}

HVVPPanel::~HVVPPanel()
{
    turnOffWhenRunning.clear();
    delete numLevels;
    delete hgtStart;
    delete hInc;
    delete xtBox;
}

void HVVPPanel::updatePanel(const QDomElement panelElement)
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
        if (name == "levels") {
            if(parameter.toInt()!=numLevels->value())
                numLevels->setValue(parameter.toInt()); }
        if(name == "hgt_start") {
            if(parameter.toFloat()!=hgtStart->value())
                hgtStart->setValue(parameter.toFloat()); }
        if(name == "hinc") {
            if(parameter.toFloat()!=hInc->value())
                hInc->setValue(parameter.toFloat()); }
        if(name == "xt") {
            if(parameter.toFloat()!=xtBox->value())
                xtBox->setValue(parameter.toFloat()); }
        child = child.nextSiblingElement();
    }
    setPanelChanged(false);
}

bool HVVPPanel::updateConfig()
{
    QDomElement element = getPanelElement();
    if(checkPanelChanged())
    {
        if(getFromElement("levels").toInt()!=numLevels->value()) {
            emit changeDom(element, QString("levels"),
                           QString().setNum(numLevels->value()));
        }
        if(fabs(getFromElement("hgt_start").toFloat()-hgtStart->value())>=0.01){
            emit changeDom(element, QString("hgt_start"),
                           QString().setNum(hgtStart->value()));
        }
        if(fabs(getFromElement("hinc").toFloat()-hInc->value())>= 0.01) {
            emit changeDom(element, QString("hinc"),
                           QString().setNum(hInc->value()));
        }
        if(fabs(getFromElement("xt").toFloat()-xtBox->value()) >= 0.1) {
            emit changeDom(element, QString("xt"),
                           QString().setNum(xtBox->value()));
        }
    }
    setPanelChanged(false);
    return true;
}

bool HVVPPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...

    // Parameters for HVVP should be adequately handled by range settings
    // in the GUI construction, limits are already build in.

    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}

PressurePanel::PressurePanel()
    :AbstractPanel()
{
    QLabel *dirLabel = new QLabel(tr("Directory Containing Pressure Data"));
    QGridLayout *dirLayout = new QGridLayout;
    dirLayout->addWidget(dirLabel, 0, 0);
    dirLayout->addWidget(dir, 1, 0, 1, 3);
    dirLayout->addWidget(browse, 1,3);

    QLabel *gradientHeightLabel = new QLabel(tr("Height at which Pressure Gradient is Calculated (km)"));
    gradientHeight = new QDoubleSpinBox;
    gradientHeight->setDecimals(2);
    gradientHeight->setRange(1,20);
    gradientHeight->setValue(1);
    QHBoxLayout *gradientHeightLayout = new QHBoxLayout;
    gradientHeightLayout->addWidget(gradientHeightLabel);
    gradientHeightLayout->addStretch();
    gradientHeightLayout->addWidget(gradientHeight);

    QLabel *pressureFormatLabel = new QLabel(tr("Data Format"));
    pressureFormatOptions = new QHash<QString, QString>;
    pressureFormatOptions->insert(QString("Select a Pressure Data Format"),
                                  QString(""));
    pressureFormatOptions->insert(QString("HWind"), QString("HWind"));
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

    QLabel *maxObsTimeLabel = new QLabel(tr("Discard Pressure Observations After (minutes)"));
    maxObsTime = new QSpinBox;
    maxObsTime->setValue(59);
    maxObsTime->setRange(0,500);
    QHBoxLayout *maxObsTimeLayout = new QHBoxLayout;
    maxObsTimeLayout->addWidget(maxObsTimeLabel);
    maxObsTimeLayout->addWidget(maxObsTime);

    QGroupBox *maxObsDistBox = new QGroupBox(tr("Maximum Distance to Pressure Observations"));
    maxObsDistCenter = new QRadioButton(tr("Measure from TC Center"),maxObsDistBox);

    maxObsDistRing = new QRadioButton(tr("Measure from Edge of Analysis"),maxObsDistBox);

    QLabel *maxObsDistLabel = new QLabel(tr("Maximum Distance (km)"));
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

    QGroupBox *intenseBox = new QGroupBox("Intensification Specifications");
    QLabel *avIntervalLabel = new QLabel(tr("Number of Volumes Averaged"));
    avInterval = new QSpinBox;
    avInterval->setRange(3,12);
    avInterval->setValue(8);
    QHBoxLayout *avIntLayout = new QHBoxLayout();
    avIntLayout->addWidget(avIntervalLabel,50);
    avIntLayout->addWidget(avInterval,0);

    QLabel *rapidLimitLabel = new QLabel(tr("Pressure Change for Warnings (mb/hr)"));
    rapidLimit = new QDoubleSpinBox;
    rapidLimit->setDecimals(1);
    rapidLimit->setRange(1,10);
    rapidLimit->setValue(3);
    QHBoxLayout *rapidLimitLayout = new QHBoxLayout;
    rapidLimitLayout->addWidget(rapidLimitLabel,50);
    rapidLimitLayout->addWidget(rapidLimit,0);
    QVBoxLayout *intenseLayout = new QVBoxLayout(intenseBox);
    intenseLayout->addLayout(rapidLimitLayout);
    intenseLayout->addLayout(avIntLayout);
    intenseBox->setLayout(intenseLayout);


    QVBoxLayout *main = new QVBoxLayout;
    main->addLayout(dirLayout);
    main->addLayout(pressureFormatLayout);
    main->addLayout(gradientHeightLayout);
    main->addLayout(maxObsTimeLayout);
    main->addWidget(maxObsDistBox);
    main->addWidget(intenseBox);
    main->addStretch(1);
    setLayout(main);

    // Add the widgets that should not be adjustable at runtime to
    // the turnOffWhenRunning QList
    turnOffWhenRunning.append(dir);
    turnOffWhenRunning.append(dirLabel);
    turnOffWhenRunning.append(browse);
    turnOffWhenRunning.append(pressureFormat);
    turnOffWhenRunning.append(pressureFormatLabel);

    connect(dir, SIGNAL(textChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(pressureFormat, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
    connect(gradientHeight, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(maxObsTime, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(maxObsDist, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(maxObsDistCenter, SIGNAL(toggled(const bool)),
            this, SLOT(valueChanged()));
    connect(maxObsDistRing, SIGNAL(toggled(const bool)),
            this, SLOT(valueChanged()));
    connect(avInterval, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(rapidLimit, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));

    setPanelChanged(false);
}

PressurePanel::~PressurePanel()
{
    turnOffWhenRunning.clear();
    delete pressureFormat;
    delete pressureFormatOptions;
    delete maxObsTime;
    delete maxObsDist;
    delete maxObsDistCenter;
    delete maxObsDistRing;
    delete avInterval;
    delete rapidLimit;
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
                emit workingDirectoryChanged();
            }
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
        if(name == "height") {
            if(parameter.toFloat()!=gradientHeight->value())
                gradientHeight->setValue(parameter.toFloat()); }
        if(name == "maxobsmethod"){
            if(parameter == "center"){
                maxObsDistCenter->setChecked(true);
            }
            if(parameter == "ring"){
                maxObsDistRing->setChecked(true);
            }
        }
        if(name == "rapidlimit") {
            if(parameter.toFloat()!=rapidLimit->value())
                rapidLimit->setValue(parameter.toFloat()); }
        if(name == "av_interval") {
            if(parameter.toInt()!=avInterval->value())
                avInterval->setValue(parameter.toInt()); }

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
        if(fabs(getFromElement("height").toFloat()-gradientHeight->value())>=0.1) {
            emit changeDom(element, QString("height"),
                           QString().setNum(gradientHeight->value()));
        }
        if(getFromElement("maxobstime").toInt()!=maxObsTime->value()) {
            emit changeDom(element, QString("maxobstime"),
                           QString().setNum(maxObsTime->value()));
        }
        if(fabs(getFromElement("maxobsdist").toFloat()-maxObsDist->value())>=0.01) {
            emit changeDom(element, QString("maxobsdist"),
                           QString().setNum(maxObsDist->value()));
        }
        if(maxObsDistCenter->isChecked())
            if(getFromElement("maxobsmethod")!=QString("center")) {
                emit changeDom(element, QString("maxobsmethod"), QString("center"));
            }
        if(maxObsDistRing->isChecked())
            if(getFromElement("maxobsmethod")!=QString("ring")) {
                emit changeDom(element, QString("maxobsmethod"), QString("ring"));
            }
        if(getFromElement("av_interval").toInt()!=avInterval->value()) {
            emit changeDom(element, QString("av_interval"),
                           QString().setNum(avInterval->value()));
        }
        if(fabs(getFromElement("rapidlimit").toFloat()-rapidLimit->value())>=0.1) {
            emit changeDom(element, QString("av_interval"),
                           QString().setNum(rapidLimit->value()));
        }
    }
    setPanelChanged(false);
    return true;
}

bool PressurePanel::setDefaultDirectory(QDir* newDir)
{
    //  Message::toScreen("PressurePanel - Set default directory");
    //  emit log(Message("PressurePanel - Set default directory"));
    QString subDirectory("pressure");
    if(!newDir->isAbsolute())
        newDir->makeAbsolute();
    if(newDir->exists(subDirectory))
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }
    if(newDir->mkdir(subDirectory)) {
        if(newDir->cd(subDirectory)){
            if(newDir->isReadable()){
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return true;
            }
            else {
                newDir->cdUp();
                defaultDirectory->cd(newDir->path());
                delete newDir;
                return false;
            }
        }
        else {
            defaultDirectory->cd(newDir->path());
            delete newDir;
            return false;
        }
    }
    else {
        defaultDirectory->cd(newDir->path());
        delete newDir;
        return false;
    }
}

bool PressurePanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...

    // Make sure that we have selected a type of pressure reading
    QString temp(pressureFormat->currentText());
    if(pressureFormatOptions->value(temp)==QString("")) {
        emit log(Message(QString(), 0, this->objectName(), Red,
                         QString(tr("Please select a pressure file type"))));
        return false;
    }

    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}


GraphicsPanel::GraphicsPanel()
    :AbstractPanel()
{

    QGroupBox *pressureType = new QGroupBox(tr("Type of Pressure Displayed"));

    QLabel *pressureLabel = new QLabel(tr("Display Absolute Pressure Estimate"));
    pressure = new QRadioButton(pressureType);
    QHBoxLayout *pressureLayout = new QHBoxLayout();
    pressureLayout->addWidget(pressure);
    pressureLayout->addWidget(pressureLabel);
    pressureLayout->addStretch();

    QLabel *deficitLabel = new QLabel(tr("Display Pressure Deficit Estimate"));
    deficit = new QRadioButton(pressureType);
    QHBoxLayout *deficitLayout = new QHBoxLayout();
    deficitLayout->addWidget(deficit);
    deficitLayout->addWidget(deficitLabel);
    deficitLayout->addStretch();

    QVBoxLayout *typeLayout = new QVBoxLayout();
    typeLayout->addLayout(pressureLayout);
    typeLayout->addLayout(deficitLayout);
    pressureType->setLayout(typeLayout);
    pressure->setChecked(true);

    graphParameters = new QGroupBox(tr("Parameters for Graph Display"));
    graphParameters->setCheckable(true);
    QVBoxLayout *graph = new QVBoxLayout;

    QLabel *pMax = new QLabel(tr("Maximum Pressure (mb)"));
    pMaxBox = new QDoubleSpinBox;
    pMaxBox->setRange(700,1100);
    pMaxBox->setDecimals(0);
    QHBoxLayout *pMaxLayout = new QHBoxLayout();
    pMaxLayout->addWidget(pMax);
    pMaxLayout->addStretch();
    pMaxLayout->addWidget(pMaxBox);

    QLabel *pMin = new QLabel(tr("Minimum Pressure (mb)"));
    pMinBox = new QDoubleSpinBox;
    pMinBox->setRange(0, 1000);
    pMinBox->setDecimals(0);
    QHBoxLayout *pMinLayout = new QHBoxLayout();
    pMinLayout->addWidget(pMin);
    pMinLayout->addStretch();
    pMinLayout->addWidget(pMinBox);

    QLabel *rmwMax = new QLabel(tr("Maximum RMW (km)"));
    rmwMaxBox = new QDoubleSpinBox;
    rmwMaxBox->setDecimals(0);
    rmwMaxBox->setRange(0,200);
    QHBoxLayout *rmwMaxLayout = new QHBoxLayout();
    rmwMaxLayout->addWidget(rmwMax);
    rmwMaxLayout->addStretch();
    rmwMaxLayout->addWidget(rmwMaxBox);

    QLabel *rmwMin = new QLabel(tr("Minimum RMW (km)"));
    rmwMinBox = new QDoubleSpinBox;
    rmwMinBox->setDecimals(0);
    rmwMinBox->setRange(0,200);
    QHBoxLayout *rmwMinLayout = new QHBoxLayout();
    rmwMinLayout->addWidget(rmwMin);
    rmwMinLayout->addStretch();
    rmwMinLayout->addWidget(rmwMinBox);

    QLabel *defMax = new QLabel(tr("Maximum Pressure Deficit (mb)"));
    defMaxBox = new QDoubleSpinBox();
    defMaxBox->setDecimals(0);
    defMaxBox->setRange(-20,200);
    QHBoxLayout *defMaxLayout = new QHBoxLayout();
    defMaxLayout->addWidget(defMax);
    defMaxLayout->addStretch();
    defMaxLayout->addWidget(defMaxBox);

    QLabel *defMin = new QLabel(tr("Minimum Pressure Deficit (mb)"));
    defMinBox = new QDoubleSpinBox();
    defMinBox->setDecimals(0);
    defMinBox->setRange(-20,200);
    QHBoxLayout *defMinLayout = new QHBoxLayout();
    defMinLayout->addWidget(defMin);
    defMinLayout->addStretch();
    defMinLayout->addWidget(defMinBox);

    QLabel *beginTimeLabel = new QLabel(tr("First Graph Display Time"));
    startDateTime = new QDateTimeEdit();
    startDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
    QDate minDate(QDate::fromString(QString("1900-01-01"),"yyyy-MM-dd"));
    QTime minTime(QTime::fromString(QString("00:00:00"), "hh:mm:ss"));
    startDateTime->setMinimumTime(minTime);
    startDateTime->setMinimumDate(minDate);
    startDateTime->setDateTime(QDateTime::currentDateTime());
    QHBoxLayout *startLayout = new QHBoxLayout();
    startLayout->addWidget(beginTimeLabel);
    startLayout->addStretch();
    startLayout->addWidget(startDateTime);

    QLabel *endTimeLabel = new QLabel (tr("Final Graph Display Time"));
    endDateTime = new QDateTimeEdit();
    endDateTime->setDisplayFormat("MMM-dd-yyyy hh:mm:ss");
    endDateTime->setMinimumTime(minTime);
    endDateTime->setMinimumDate(minDate);
    endDateTime->setDateTime(QDateTime::currentDateTime().addSecs(3*3600));
    QHBoxLayout *endLayout = new QHBoxLayout();
    endLayout->addWidget(endTimeLabel);
    endLayout->addStretch();
    endLayout->addWidget(endDateTime);


    graph->addLayout(pMaxLayout);
    graph->addLayout(pMinLayout);
    graph->addLayout(rmwMaxLayout);
    graph->addLayout(rmwMinLayout);
    graph->addLayout(defMaxLayout);
    graph->addLayout(defMinLayout);
    graph->addLayout(startLayout);
    graph->addLayout(endLayout);
    graphParameters->setLayout(graph);
    graphParameters->setChecked(false);
    QVBoxLayout *main = new QVBoxLayout;
    main->addWidget(pressureType);
    main->addWidget(graphParameters);
    main->addStretch(1);
    setLayout(main);

    connect(pressure, SIGNAL(toggled(bool)),
            this, SLOT(valueChanged()));
    connect(deficit, SIGNAL(toggled(bool)),
            this, SLOT(valueChanged()));
    connect(graphParameters, SIGNAL(toggled(bool)),
            this, SLOT(valueChanged()));
    connect(pMaxBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(pMinBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(rmwMaxBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(rmwMinBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(defMaxBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(defMinBox, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(startDateTime, SIGNAL(dateTimeChanged(const QDateTime&)),
            this, SLOT(valueChanged()));
    connect(endDateTime, SIGNAL(dateTimeChanged(const QDateTime&)),
            this, SLOT(valueChanged()));

    setPanelChanged(false);
}

GraphicsPanel::~GraphicsPanel()
{
    turnOffWhenRunning.clear();
    delete pMaxBox;
    delete pMinBox;
    delete rmwMaxBox;
    delete rmwMinBox;
    delete defMaxBox;
    delete defMinBox;
    delete startDateTime;
    delete endDateTime;
    delete graphParameters;
    delete pressure;
    delete deficit;
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
        if (name == "defmin") {
            defMinBox->setValue(parameter.toDouble()); }
        if (name == "defmax") {
            defMaxBox->setValue(parameter.toDouble()); }
        if (name == "startdate") {
            startDateTime->setDate(QDate::fromString(parameter, "yyyy-MM-dd")); }
        if (name == "enddate") {
            endDateTime->setDate(QDate::fromString(parameter, "yyyy-MM-dd")); }
        if (name == "starttime") {
            startDateTime->setTime(QTime::fromString(parameter, "hh:mm:ss")); }
        if (name == "endtime") {
            endDateTime->setTime(QTime::fromString(parameter, "hh:mm:ss")); }
        if (name == "display_type") {
            if(parameter == "deficit")
                deficit->setChecked(true);
            else
                pressure->setChecked(true);
        }
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
        emit stateChange(QString("show_pressure"), pressure->isChecked());
        if(graphParameters->isChecked()) {
            emit changeDom(element, QString("pressmin"),
                           QString().setNum(pMinBox->value()));

            emit changeDom(element, QString("pressmax"),
                           QString().setNum(pMaxBox->value()));

            emit changeDom(element, QString("rmwmin"),
                           QString().setNum(rmwMinBox->value()));

            emit changeDom(element, QString("rmwmax"),
                           QString().setNum(rmwMaxBox->value()));

            emit changeDom(element, QString("defmin"),
                           QString().setNum(defMinBox->value()));

            emit changeDom(element, QString("defmax"),
                           QString().setNum(defMaxBox->value()));

            emit changeDom(element, QString("startdate"),
                           startDateTime->date().toString("yyyy-MM-dd"));

            emit changeDom(element, QString("enddate"),
                           endDateTime->date().toString("yyyy-MM-dd"));

            emit changeDom(element, QString("starttime"),
                           startDateTime->time().toString("hh:mm:ss"));

            emit changeDom(element, QString("endtime"),
                           endDateTime->time().toString("hh:mm:ss"));
            emit changeDom(element, QString("autolimits"),QString("on"));
        }
        else {
            emit changeDom(element, QString("autolimits"),QString("off"));
        }
        if(pressure->isChecked()) {
            emit changeDom(element, QString("display_type"), QString("pressure"));
        }
        else {
            emit changeDom(element, QString("display_type"), QString("deficit"));
        }
    }
    setPanelChanged(false);
    return true;
}

bool GraphicsPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...

    // All the values used are sufficiently specified within the
    // construction of their GUI components, no need here yet.

    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}

bool GraphicsPanel::checkDisplayType()
{
    emit stateChange(QString("show_pressure"), pressure->isChecked());
    if(pressure->isChecked())
        return true;
    else
        return false;
}

QCPanel::QCPanel()
    :AbstractPanel()
{
    QGroupBox *qcParameters = new QGroupBox(tr("Quality Control Parameters"));

    QLabel *velMinThresLabel = new QLabel(tr("Ignore Gates with Velocity Magnitudes Less Than (m/s)"));
    velocityMinimum = new QDoubleSpinBox;
    velocityMinimum->setDecimals(3);
    velocityMinimum->setRange(0,10);
    velocityMinimum->setValue(1.5);

    QLabel *velMaxThresLabel = new QLabel(tr("Ignore Gates with Velocity Magnitudes Greater Than (m/s)"));
    velocityMaximum = new QDoubleSpinBox;
    velocityMaximum->setDecimals(3);
    velocityMaximum->setRange(1,999);
    velocityMaximum->setValue(100);

    QLabel *refMinThresLabel = new QLabel(tr("Ignore Gates with Reflectivity Values Less Than (dBZ)"));
    reflectivityMinimum = new QDoubleSpinBox;
    reflectivityMinimum->setDecimals(3);
    reflectivityMinimum->setRange(-500,500);
    reflectivityMinimum->setValue(-15);

    QLabel *refMaxThresLabel = new QLabel(tr("Ignore Gates with Reflectivity Values Greater Than (dBZ)"));
    reflectivityMaximum = new QDoubleSpinBox;
    reflectivityMaximum->setDecimals(3);
    reflectivityMaximum->setRange(-500,500);
    reflectivityMaximum->setValue(65);

    QLabel *specThresLabel = new QLabel(tr("Ignore Gates with Spectrum Width Greater Than (m/s)"));
    spectralThreshold = new QDoubleSpinBox;
    spectralThreshold->setDecimals(2);
    spectralThreshold->setRange(0,50);
    spectralThreshold->setValue(12);

    QLabel *bbLabel = new QLabel(tr("Number of Gates Averaged for Velocity Dealiasing"));
    bbSegmentSize = new QSpinBox;
    bbSegmentSize->setRange(1,150);
    bbSegmentSize->setValue(30);

    QLabel *maxFoldLabel = new QLabel(tr("Maximum Number of Folds Possible in Velocity Dealiasing"));
    maxFoldCount = new QSpinBox;
    maxFoldCount->setRange(0,20);
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
    gvad = new QRadioButton(tr("Use GVAD Algorithm to estimate wind at radar"), findWind);
    vad = new QRadioButton(tr("Use VAD Algorithm to estimate wind at radar"), findWind);
    user = new QRadioButton(tr("Enter boundary layer wind at radar manually"), findWind);
    //  known = new QRadioButton(tr("Use Available AWIPS Data"), findWind);

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

    QLabel *vadthrLabel = new QLabel(tr("Minimum Number of Points for VAD Processing"));
    vadthr = new QSpinBox;
    vadthr->setRange(1,360);
    vadthr->setValue(30);
    QHBoxLayout *vadthrLayout = new QHBoxLayout;
    vadthrLayout->addSpacing(20);
    vadthrLayout->addWidget(vadthrLabel);
    vadthrLayout->addWidget(vadthr);

    QVBoxLayout *vadLayout = new QVBoxLayout;
    vadLayout->addLayout(vadLevelsLayout);
    vadLayout->addLayout(numCoLayout);
    vadLayout->addLayout(vadthrLayout);
    vadParameters->setLayout(vadLayout);
    vadParameters->hide();

    QFrame *gvadParameters = new QFrame;

    QLabel *gvadthrLabel = new QLabel(tr("Minimum Number of Points for GVAD Processing"));
    gvadthr = new QSpinBox;
    gvadthr->setRange(1,360);
    gvadthr->setValue(180);

    QHBoxLayout *gvadthrLayout = new QHBoxLayout;
    gvadthrLayout->addSpacing(20);
    gvadthrLayout->addWidget(gvadthrLabel);
    gvadthrLayout->addWidget(gvadthr);

    QVBoxLayout *gvadLayout = new QVBoxLayout;
    gvadLayout->addLayout(gvadthrLayout);
    gvadParameters->setLayout(gvadLayout);
    gvadParameters->hide();

    QFrame *userParameters = new QFrame;

    QLabel *windSpeedLabel = new QLabel(tr("Reference Wind Speed (m/s)"));
    windSpeed = new QDoubleSpinBox;
    windSpeed->setDecimals(2);
    windSpeed->setRange(0, 200);
    QHBoxLayout *windSpeedLayout = new QHBoxLayout;
    windSpeedLayout->addSpacing(20);
    windSpeedLayout->addWidget(windSpeedLabel);
    windSpeedLayout->addWidget(windSpeed);

    QLabel *windDirectionLabel = new QLabel(tr("Reference Wind Direction (degrees from North)"));
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

    /*
   * QFrame *knownParameters = new QFrame;
   * QLabel *knownDirLabel = new QLabel(tr("AWIPS Data Directory"));
   * QGridLayout *knownDirLayout = new QGridLayout();
   * QHBoxLayout *knownLayout = new QHBoxLayout;
   * knownDirLayout->addWidget(knownDirLabel, 0, 0);
   * knownDirLayout->addWidget(dir, 1, 0, 1, 3);
   * knownDirLayout->addWidget(browse, 1, 3);
   * knownLayout->addSpacing(20);
   * knownLayout->addLayout(knownDirLayout);
   * knownParameters->setLayout(knownLayout);
   * knownParameters->hide();
   */

    QVBoxLayout *findWindLayout = new QVBoxLayout;
    findWindLayout->addWidget(gvad);
    findWindLayout->addWidget(gvadParameters);
    findWindLayout->addWidget(vad);
    findWindLayout->addWidget(vadParameters);
    findWindLayout->addWidget(user);
    findWindLayout->addWidget(userParameters);
    //  findWindLayout->addWidget(known);
    //  findWindLayout->addWidget(knownParameters);
    findWind->setLayout(findWindLayout);

    QVBoxLayout *main = new QVBoxLayout;
    main->addWidget(qcParameters);
    main->addWidget(findWind);
    main->addStretch(1);
    setLayout(main);

    connect(bbSegmentSize, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(maxFoldCount, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(velocityMinimum, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(velocityMaximum, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(reflectivityMinimum, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(reflectivityMaximum, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(spectralThreshold, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));

    connect(gvad, SIGNAL(toggled(const bool)),
            this, SLOT(valueChanged()));
    connect(gvad, SIGNAL(toggled(bool)),
            gvadParameters, SLOT(setVisible(bool)));
    connect(vad, SIGNAL(toggled(const bool)),
            this, SLOT(valueChanged()));
    connect(vad, SIGNAL(toggled(bool)),
            vadParameters, SLOT(setVisible(bool)));
    connect(vadLevels, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(numCoefficients, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(vadthr, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(gvadthr, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));

    connect(user, SIGNAL(toggled(const bool)),
            this, SLOT(valueChanged()));
    connect(user, SIGNAL(toggled(bool)),
            userParameters, SLOT(setVisible(bool)));
    connect(windSpeed, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));
    connect(windDirection, SIGNAL(valueChanged(const QString&)),
            this, SLOT(valueChanged()));

    /*
   * connect(known, SIGNAL(toggled(const bool)),
   *	  this, SLOT(valueChanged()));
   * connect(known, SIGNAL(toggled(bool)),
   *	  knownParameters, SLOT(setVisible(bool)));
   * connect(dir, SIGNAL(textChanged(const QString&)),
   *  this, SLOT(valueChanged()));
   */

    setPanelChanged(false);

}

QCPanel::~QCPanel()
{
    turnOffWhenRunning.clear();
    delete vad;
    delete user;
    //delete known;
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
            if(parameter == "gvad")
                gvad->setChecked(true);
            if(parameter == "vad")
                vad->setChecked(true);
            if(parameter == "user")
                user->setChecked(true);
            //      if(parameter == "known")
            //	known->setChecked(true);
        }
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
        /*
    if(name == "awips_dir") {
      dir->clear();
      dir->insert(parameter); }
    */
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
            if(getFromElement("wind_method")!=QString("vad")) {
                emit changeDom(element, QString("wind_method"), QString("vad"));
            }
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
        }
        if(gvad->isChecked()) {
            if(getFromElement("wind_method")!=QString("gvad")) {
                emit changeDom(element, QString("wind_method"), QString("gvad"));
            }
            if(getFromElement("gvadthr").toInt()!=gvadthr->value()) {
                emit changeDom(element, QString("gvadthr"),
                               QString().setNum(gvadthr->value()));
            }
        }
        if(user->isChecked()) {
            if(getFromElement("wind_method")!=QString("user")) {
                emit changeDom(element, QString("wind_method"), QString("user"));
            }
            if(getFromElement("windspeed").toDouble()!= windSpeed->value()) {
                emit changeDom(element, QString("windspeed"),
                               QString().setNum(windSpeed->value()));
            }
            if(getFromElement("winddirection").toDouble()!=windDirection->value()){
                emit changeDom(element, QString("winddirection"),
                               QString().setNum(windDirection->value()));
            }
        }
        /*
 if(known->isChecked()) {
 emit changeDom(element, QString("wind_method"), QString("known"));

 if(getFromElement("awips_dir")!= dir->text()) {
 emit changeDom(element, QString("awips_dir"),dir->text());
 }
 }
      */
        setPanelChanged(false);
    }
    return true;
}

bool QCPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...

    // All of the values used in this panel are adequately constrained by
    // the limits in constructing their GUI counterparts, nothing to check
    // right now.

    emit log(Message(QString(), 0, this->objectName(), Green));
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
