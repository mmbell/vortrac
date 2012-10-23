/*
 * AnalysisPage.cpp
 * VORTRAC
 *
 * Created by Michael Bell on 7/21/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include <QtGui>
#include <QtNetwork>
#include "AnalysisPage.h"
#include "Message.h"
#include "thredds_Config.h"

AnalysisPage::AnalysisPage(QWidget *parent)
    : QWidget(parent)
{
    this->setObjectName("Analysis Display Page");
    QFont f("Helvetica", 12, QFont::Bold);
    setFont(f);
    lastMax = -999;
    statusLog = new Log();

    connect(this, SIGNAL(log(const Message&)),statusLog, SLOT(catchLog(const Message&)));
    connect(statusLog, SIGNAL(redLightAbort()),this, SLOT(abortThread()));

    // Create the widgets needed to interact with the log

    diagPanel = new DiagnosticPanel;
    diagPanel->setFixedWidth(250);
    connect(diagPanel, SIGNAL(log(const Message&)),
            this,      SLOT(catchLog(const Message&)));
    connect(statusLog, SIGNAL(newStopLightColor(StopLightColor, const QString)),
            diagPanel, SLOT(changeStopLight(StopLightColor, const QString)));
    connect(statusLog, SIGNAL(newStormSignalStatus(StormSignalStatus,const QString)),
            diagPanel, SLOT(changeStormSignal(StormSignalStatus,const QString)));

    // Create a new configuration instance
    configData = new Configuration;
    connect(configData, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));

    // Define all the widgets
    configDialog = new ConfigurationDialog(this, configData);
    connect(configDialog, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));

    QTabWidget *visuals = new QTabWidget;
    // For reasons I can't figure out, this is causing a problem displaying the labels in Qt 4.1.3 and 4.1.4
    //visuals->setTabPosition(QTabWidget::West);

    GraphFace *graph = new GraphFace();
    graph->updateTitle(this, vortexLabel);
    connect(graph, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));

    cappiDisplay = new CappiDisplay();
    cappiDisplay->clearImage();

    imageFileName = graph->getImageFileName();


    // Connect signals and slots that update the configuration

    connect(configData, SIGNAL(configChanged()), this, SLOT(updatePage()));

    connect(this, SIGNAL(tabLabelChanged(QWidget*, const QString&)),graph, SLOT(updateTitle(QWidget*, const QString&)));


    // Construct Analysis Page Layout
    deficitLabel = new QLabel(tr("Pressure Deficit From 0 km (mb):"));
    currDeficit = new QLabel(tr("0"));
    currDeficit->setFrameStyle(QFrame::StyledPanel);
    currDeficit->resize(100,100);
    QFont font = currDeficit->font();
    font.setPointSize(36);
    currDeficit->setFont(font);
    
    QLabel *pressureLabel = new QLabel(tr("Current Pressure (mb):"));
    currPressure = new QLabel(tr("0"));
    currPressure->setFrameStyle(QFrame::StyledPanel);
    currPressure->resize(100,100);
    currPressure->setFont(font);
    
    QLabel *rmwLabel = new QLabel(tr("Current RMW (km):"));
    currRMW = new QLabel(tr("0"));
    currRMW->setFrameStyle(QFrame::StyledPanel);
    currRMW->resize(100,100);
    currRMW->setFont(font);
    
    QGroupBox *pressureGraph = new QGroupBox;

    QPushButton *legend = new QPushButton("See Legend");
    QPushButton *saveGraph = new QPushButton("Save Graph");

    //--For Demo
    QHBoxLayout *subLayout = new QHBoxLayout;
    subLayout->addWidget(legend);
    subLayout->addWidget(saveGraph);

    QVBoxLayout *layout = new QVBoxLayout(pressureGraph);
    layout->addWidget(graph, 100);
    layout->addLayout(subLayout);
    pressureGraph->setLayout(layout);
    visuals->addTab(pressureGraph, "Pressure/RMW");

    connect(saveGraph, SIGNAL(clicked()),graph, SLOT(saveImage()));
    connect(legend, SIGNAL(clicked()), graph, SLOT(makeKey()));
    connect(this, SIGNAL(vortexListChanged(VortexList*)),graph, SLOT(newInfo(VortexList*)));
    connect(configDialog, SIGNAL(stateChange(const QString&,const bool)),graph, SLOT(manualAxes(const QString&, const bool)));
    connect(configDialog, SIGNAL(changeGraphicsParameter(const QString&, const float)),graph,SLOT(manualParameter(const QString&,const float)));
    connect(configDialog, SIGNAL(changeGraphicsParameter(const QString&, const QString&)),graph, SLOT(manualParameter(const QString&, const QString&)));


    //initialize cappi display
    appMaxWind = new QLabel();
    appMaxWind->setFrameStyle(QFrame::StyledPanel);
    appMaxWind->resize(100,100);
    font = appMaxWind->font();
    font.setPointSize(24);
    appMaxWind->setFont(font);
    appMaxWind->setText(QString().setNum(cappiDisplay->getMaxApp(),'f', 1));
    QLabel* appMaxLabel = new QLabel(tr("Maximum Approaching Wind (m/s)"));
    appMaxLabel2 = new QLabel();

    recMaxWind = new QLabel();
    recMaxWind->setFrameStyle(QFrame::StyledPanel);
    recMaxWind->resize(100,100);
    recMaxWind->setFont(font);
    recMaxWind->setText(QString().setNum(cappiDisplay->getMaxRec(),'f', 1));
    QLabel* recMaxLabel = new QLabel(tr("Maximum Receding Wind (m/s)"));
    recMaxLabel2 = new QLabel();
    QLabel* emptyLabel = new QLabel();
    QLabel* emptyLabel2 = new QLabel();

    lcdCenterLat = new QLabel();
    lcdCenterLat->setFrameStyle(QFrame::StyledPanel);
    lcdCenterLat->resize(100,100);
    lcdCenterLat->setFont(font);
    lcdCenterLat->setText(QString().setNum(0));
    QLabel* lcdCenterLatLabel = new QLabel(tr("VORTRAC TC Latitude"));

    lcdCenterLon = new QLabel();
    lcdCenterLon->setFrameStyle(QFrame::StyledPanel);
    lcdCenterLon->resize(100,100);
    lcdCenterLon->setFont(font);
    lcdCenterLon->setText(QString().setNum(0));
    QLabel* lcdCenterLonLabel = new QLabel(tr("VORTRAC TC Longitude"));

    lcdUserCenterLat = new QLabel();
    lcdUserCenterLat->setFrameStyle(QFrame::StyledPanel);
    lcdUserCenterLat->resize(100,100);
    lcdUserCenterLat->setFont(font);
    lcdUserCenterLat->setText(QString().setNum(0));
    QLabel* lcdUserCenterLatLabel = new QLabel(tr("User Estimated TC Latitude"));

    lcdUserCenterLon = new QLabel();
    lcdUserCenterLon->setFrameStyle(QFrame::StyledPanel);
    lcdUserCenterLon->resize(100,100);
    lcdUserCenterLon->setFont(font);
    lcdUserCenterLon->setText(QString().setNum(0));
    QLabel* lcdUserCenterLonLabel = new QLabel(tr("User Estimated TC Longitude"));

    QGroupBox *fieldDisplay = new QGroupBox(tr("Radar display"));
    QRadioButton *velocity = new QRadioButton(tr("Velocity"), fieldDisplay);
    QRadioButton *reflectivity = new QRadioButton(tr("dBZ"), fieldDisplay);
    velocity->setChecked(true);

    QVBoxLayout *appRecLayout = new QVBoxLayout();
    appRecLayout->addStretch();
    appRecLayout->addWidget(appMaxLabel, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(appMaxLabel2, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(appMaxWind, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(recMaxLabel, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(recMaxLabel2, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(recMaxWind, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(emptyLabel, 100, Qt::AlignHCenter);
    appRecLayout->addWidget(lcdCenterLatLabel, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(lcdCenterLat, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(lcdCenterLonLabel, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(lcdCenterLon, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(emptyLabel2, 100, Qt::AlignHCenter);
    appRecLayout->addWidget(lcdUserCenterLatLabel, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(lcdUserCenterLat, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(lcdUserCenterLonLabel, 0, Qt::AlignHCenter);
    appRecLayout->addWidget(lcdUserCenterLon, 0, Qt::AlignHCenter);
    appRecLayout->addStretch();

    QVBoxLayout *fieldLayout = new QVBoxLayout;
    fieldLayout->addWidget(velocity);
    fieldLayout->addWidget(reflectivity);
    fieldDisplay->setLayout(fieldLayout);

    QScrollArea* cappiBox = new QScrollArea;
    QHBoxLayout *cappiLayout = new QHBoxLayout(cappiBox);
    cappiLayout->addLayout(appRecLayout);
    cappiLayout->addStretch();
    cappiLayout->addWidget(cappiDisplay);
    cappiLayout->addWidget(fieldDisplay);
    cappiLayout->addStretch();

    cappiBox->setLayout(cappiLayout);
    visuals->addTab(cappiBox, "Current Cappi");
    connect(cappiDisplay, SIGNAL(hasImage(bool)),this, SLOT(updateCappiDisplay(bool)));

    statusText = new QTextEdit;
    statusText->setReadOnly(true);
    statusText->setFixedHeight(100);
    QToolButton *statusPopup = new QToolButton;
    statusPopup->setArrowType(Qt::UpArrow);
    QHBoxLayout *statusLayout = new QHBoxLayout;
    statusLayout->addWidget(statusText);
    statusLayout->addWidget(statusPopup);
    
    QLabel *hLine = new QLabel;
    hLine->setFrameStyle(QFrame::HLine);
    hLine->setLineWidth(2);

    runButton = new QPushButton("&Run");
    abortButton = new QPushButton("&Abort");
    runButton->setEnabled(true);
    abortButton->setEnabled(false);
    QProgressBar *progressBar = new QProgressBar;
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->reset();

    // Lay them out
    QHBoxLayout *lcdLayout = new QHBoxLayout;
    lcdLayout->addStretch();
    lcdLayout->addWidget(deficitLabel);
    lcdLayout->addWidget(currDeficit);
    lcdLayout->addStretch();
    lcdLayout->addWidget(pressureLabel);
    lcdLayout->addWidget(currPressure);
    lcdLayout->addStretch();
    lcdLayout->addWidget(rmwLabel);
    lcdLayout->addWidget(currRMW);
    lcdLayout->addStretch();

    QHBoxLayout *runLayout = new QHBoxLayout;
    runLayout->addWidget(progressBar);
    runLayout->addWidget(runButton);
    runLayout->addWidget(abortButton);

    QGridLayout *displayPanel = new QGridLayout;
    displayPanel->addLayout(lcdLayout,0,0);
    displayPanel->addWidget(visuals,1,0);
    displayPanel->addWidget(hLine,2,0);
    displayPanel->addLayout(statusLayout,3,0);
    displayPanel->addLayout(runLayout,4,0);
    displayPanel->setRowMinimumHeight(0,50);

    QHBoxLayout *display = new QHBoxLayout;
    display->addWidget(diagPanel);
    display->addLayout(displayPanel);

    setLayout(display);

    // Connect the run and abort buttons
    connect(runButton, SIGNAL(clicked()), this, SLOT(runThread()));
    connect(abortButton, SIGNAL(clicked()), this, SLOT(abortThread()));

    // Connect a log to the text window and progress bar
    connect(statusLog, SIGNAL(newLogEntry(QString)),this, SLOT(updateStatusLog(QString)));
    connect(statusLog, SIGNAL(newProgressEntry(int)),progressBar, SLOT(setValue(int)));

    connect(statusText, SIGNAL(textChanged()),this, SLOT(autoScroll()));
    connect(statusPopup, SIGNAL(clicked()), this, SLOT(popUpStatus()));

    statusLog->catchLog(Message("VORTRAC Status Log for "+QDateTime::currentDateTime().toUTC().toString()+ " UTC"));

    pollThread = NULL;
    atcf = NULL;
    madis = NULL;
    fetchremote = NULL;
    
    connect(this, SIGNAL(saveGraphImage(const QString&)),graph, SLOT(saveImage(const QString&)));

    quickstart = new StartDialog(this, configData);
    connect(quickstart, SIGNAL(go()), this, SLOT(runThread()));
    connect(quickstart, SIGNAL(manual()), this, SLOT(openConfigDialog()));

    // Connect the cappi toggle buttons
    connect(velocity, SIGNAL(toggled(const bool)),
            cappiDisplay, SLOT(toggleRadarDisplay()));

}

AnalysisPage::~AnalysisPage()
{
    prepareToClose();
    if(abortButton->isEnabled())
        abortThread();
    delete cappiDisplay;
    delete configData;
    delete diagPanel;
    delete statusLog;
    delete statusText;
    delete configDialog;
    delete currPressure;
    delete currRMW;
    delete currDeficit;
    delete deficitLabel;
    delete quickstart;
}

void AnalysisPage::newFile()
{
    // Load the default configuration
    QString resources = QCoreApplication::applicationDirPath() + "/../Resources";
    QDir resourceDir = QDir(resources);
    if(resourceDir.exists("vortrac_default.xml")) {
        if (!loadFile(resourceDir.filePath("vortrac_default.xml")))
            emit log(Message(QString("Couldn't load default configuration - Please check the default file vortrac_default.xml and insure that it is accesible"),0,this->objectName(),Red));
    }
    else {
        emit log(Message(QString("Couldn't locate default configuration - Please locate vortrac_default.xml and use the open button"),0,this->objectName(),Red));
    }
    // Set the current filename to untitled
    isUntitled = true;
    configFileName = QDir::current().filePath("vortrac_newconfig.xml");
    
    quickstart->show();

}

bool AnalysisPage::loadFile(const QString &fileName)
{
    // Load up a configuration file in the ConfigurationDialog
    if (!configData->read(fileName)) {
        emit log(Message(QString("Couldn't load configuration file"),0,this->objectName()));
        return false;
    }

    configDialog->read();

    // Set the filename
    configFileName = fileName;
    QString directoryString(configData->getParam(configData->getConfig("vortex"),
                                                 "dir"));
    workingDirectory = QDir(directoryString);
    //Message::toScreen(workingDirectory.path());
    if(!workingDirectory.isAbsolute()) {
        workingDirectory.makeAbsolute();
    }
    // Check to make sure the workingDirectory exists
    if(!workingDirectory.exists())
        if(!workingDirectory.mkpath(directoryString)) {
            emit log(Message(QString("Failed to find or create working directory path: "+directoryString),0,this->objectName()));
            return false;
        }
    //Message::toScreen("2"+workingDirectory.path());
    statusLog->setWorkingDirectory(workingDirectory);
    //graph->setWorkingDirectory(workingDirectory);
    isUntitled = false;
    return true;
}

bool AnalysisPage::save()
{

    // Save the current configuration
    if (isUntitled) {
        return saveAs();
    } else {
        return saveFile(configFileName);
    }

}

bool AnalysisPage::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                    configFileName);
    if (fileName.isEmpty())
        return false;


    return saveFile(fileName);

}

bool AnalysisPage::saveFile(const QString &fileName)
{

    if (!configData->write(fileName)) {
        emit log(Message(QString("Failed to save configuration file to "+fileName),
                         0,this->objectName(),Yellow,QString("Unable to Save")));
        return false;
    }

    QString noExtensionFileName = fileName;

    if (noExtensionFileName.contains('.')) {
        int dotIndex = noExtensionFileName.indexOf('.');
        int drop = noExtensionFileName.count()-dotIndex+1;
        noExtensionFileName.remove(dotIndex,drop);
    }
    if (noExtensionFileName.contains('/')){
        int slashIndex = noExtensionFileName.lastIndexOf('/');
        noExtensionFileName.remove(0,slashIndex+1);
    }

    //Message::toScreen("No extension file name is ... "+noExtensionFileName);

    if(workingDirectory.exists()) {
        emit log(Message(QString("AnalysisPage: Saving copies of .log and .png in the working directory: "+workingDirectory.filePath(noExtensionFileName)),0,this->objectName()));
        if(!statusLog->saveLogFile(workingDirectory.filePath(noExtensionFileName+".log")))
            emit log(Message(QString("Failed to save .log file in "+workingDirectory.path()),0,this->objectName()));

        emit saveGraphImage(workingDirectory.filePath(noExtensionFileName+".png"));
    }
    else {
        emit log(Message(QString("Cannot find "+workingDirectory.path()+", Cannot save log & image files"),0,this->objectName(),Yellow,QString("Couldn't Save Log")));
    }
    configFileName = fileName;

    return true;

}

void AnalysisPage::setVortexLabel()
{

    QString vortexName = configData->getParam(configData->getConfig("vortex"),
                                              "name");
    QString radarName = configData->getParam(configData->getConfig("radar"),
                                             "name");
    QString underscore = "_";
    vortexLabel = vortexName + underscore + radarName;
    emit tabLabelChanged(this,vortexLabel);

}

void AnalysisPage::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
    {
        QString autoLogName = statusLog->getLogFileName();
        QFile autoLog(workingDirectory.filePath(autoLogName));
        autoLog.remove();
        event->accept();
    }
    else
        event->ignore();
}

bool AnalysisPage::maybeSave()
{
    if (configData->checkModified()) {
        int ret = QMessageBox::warning(this, tr("VORTRAC"),
                                       tr("'%1' has been modified.\n"
                                          "Do you want to save your changes?")
                                       .arg((configFileName)),
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No,
                                       QMessageBox::Cancel | QMessageBox::Escape);
        if (ret == QMessageBox::Yes)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void AnalysisPage::updatePage()
{
    // Can add more here as need be
    setVortexLabel();
    QString temp = getVortexLabel();
    emit(tabLabelChanged(this,temp));
    //Message::toScreen("updatepage before "+workingDirectory.path());
    QString newPathString(configData->getParam(configData->getConfig("vortex"),
                                               "dir"));
    QDir newPath(newPathString);
    if(newPath.isRelative())
        newPath.makeAbsolute();
    if(newPath!=workingDirectory) {
        workingDirectory = newPath;
        statusLog->setWorkingDirectory(workingDirectory);
        //graph->setWorkingDirectory(workingDirectory);

    }
    //Message::toScreen("updatePage after "+workingDirectory.path());
}

void AnalysisPage::runThread()
{
    //check the sanity of parameter
    configDialog->checkConfig();
    if(!configDialog->checkPanels()) {
        QMessageBox errCfg;
        errCfg.setText("Please check errors: Incorrect entry in the configuration");
        errCfg.exec();
        return;
    }

    //set button to correct status
    runButton->setEnabled(false);
    abortButton->setEnabled(true);

    configDialog->turnOffMembers(true);

    emit log(Message(QString(),0,this->objectName(),AllOff,QString(),Ok, QString()));

    pollThread = new workThread();

    connect(pollThread, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));
    connect(pollThread, SIGNAL(newVCP(const int)),diagPanel, SLOT(updateVCP(const int)));
    connect(pollThread, SIGNAL(newCappi(GriddedData)),cappiDisplay, SLOT(constructImage(GriddedData)),Qt::DirectConnection);

    connect(pollThread, SIGNAL(newCappiInfo(float, float, float, float, float, float, float ,float ,float, float)),
            this, SLOT(updateCappiInfo(float, float, float, float, float, float, float ,float ,float, float)),Qt::DirectConnection);
    connect(pollThread, SIGNAL(newCappiInfo(float, float, float, float, float, float, float ,float ,float, float)),
            cappiDisplay, SLOT(setGBVTDResults(float, float, float, float, float, float, float ,float ,float, float)),Qt::DirectConnection);
    connect(pollThread, SIGNAL(vortexListUpdate(VortexList*)),this, SLOT(pollVortexUpdate(VortexList*)),Qt::DirectConnection);

    atcf = new ATCF(configData);
    connect(atcf, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));
    connect(atcf, SIGNAL(tcvitalsReady()),this, SLOT(updateTcvitals()));
    pollThread->setATCF(atcf);
    
    madis = new MADISFactory(configData);
    connect(madis, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));
    connect(this, SIGNAL(updateMadis(float, float)),madis, SLOT(setBoundingBox(float, float)));

    fetchremote = new FetchRemote(configData);
    connect(fetchremote, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));

    // Check to see if there are old list files that we want to start from in
    // the working directory. We have to do this here because we cannot create
    // widgets in the threads.
    QDir    workingDirectory(configData->getParam(configData->getConfig("vortex"),"dir"));
    QString vortexName = configData->getParam(configData->getConfig("vortex"), "name");
    QString radarName = configData->getParam(configData->getConfig("radar"), "name");
    QString nameFilter = vortexName+"_"+radarName+"_";
    QStringList allPossibleFiles = workingDirectory.entryList(QDir::Files);
    allPossibleFiles = allPossibleFiles.filter(nameFilter, Qt::CaseInsensitive);
    bool continuePreviousRun = false;
    QString openOldMessage = QString(tr("Vortrac has found information about a previous run. Continuing will erase this data!\nPress 'Yes' to continue."));
    if(allPossibleFiles.count()> 0){
        if(allPossibleFiles.filter("vortexList").count()>0 && allPossibleFiles.filter("simplexList").count()>0)
        {
            int answer = QMessageBox::information(this,tr("VORTRAC"),openOldMessage,3,4,0);
            if(answer==3) {
                continuePreviousRun = false;
            } else {
                return;
	    }
        }
    }
    //set a flag in pollThread if continue previous run or not
	// Try to fetch new radar data every 5 minutes
    QString mode = configData->getParam(configData->getConfig("vortex"), "mode");
    if (mode == "operational") {
        QTimer::singleShot(0, fetchremote, SLOT(fetchRemoteData()));
        QTimer *fetchTimer = new QTimer(this);
        connect(fetchTimer, SIGNAL(timeout()), fetchremote, SLOT(fetchRemoteData()));
        fetchTimer->start(300000);
        
        QTimer::singleShot(0, atcf, SLOT(getTcvitals()));
        QTimer *atcfTimer = new QTimer(this);
        connect(atcfTimer, SIGNAL(timeout()), atcf, SLOT(getTcvitals()));
        atcfTimer->start(3600000);
        
        QTimer::singleShot(0, madis, SLOT(getPressureObs()));
        QTimer *madisTimer = new QTimer(this);
        connect(madisTimer, SIGNAL(timeout()), madis, SLOT(getPressureObs()));
        madisTimer->start(1800000);
        
	} else {
        pollThread->setContinuePreviousRun(continuePreviousRun);
        pollThread->setConfig(configData);
        pollThread->start();
    }
}

void AnalysisPage::updateTcvitals()
{
    // Get info from ATCF
    QDomElement vortex = configData->getConfig("vortex");
    QString vortexName = atcf->getStormName();
    configData->setParam(vortex, "name", vortexName);
    QString lat = QString().setNum(atcf->getLatitude(atcf->getTime()));
    configData->setParam(vortex, "lat", lat);
    QString lon = QString().setNum(atcf->getLongitude(atcf->getTime()));
    configData->setParam(vortex, "lon", lon);
    QString dir = QString().setNum(atcf->getDirection());
    configData->setParam(vortex, "direction", dir);
    QString speed = QString().setNum(atcf->getSpeed());
    configData->setParam(vortex, "speed", speed);
    QString rmw = QString().setNum(atcf->getRMW());
    configData->setParam(vortex, "rmw", rmw);
    QString date = atcf->getTime().toString("yyyy-MM-dd");
    configData->setParam(vortex, "obsdate", date);
    QString time = atcf->getTime().toString("hh:mm:ss");
    configData->setParam(vortex, "obstime", time);
    
    QDomElement radar = configData->getConfig("radar");
    configData->setParam(radar, "startdate", date);
    configData->setParam(radar, "starttime", time);
    
    QDomElement choosecenter = configData->getConfig("choosecenter");
    configData->setParam(choosecenter, "startdate", date);
    configData->setParam(choosecenter, "starttime", time);

    configDialog->checkConfig();
    if(!configDialog->checkPanels()) {
        QMessageBox errCfg;
        errCfg.setText("Please check errors: Incorrect entry in the configuration");
        errCfg.exec();
        return;
    }
    if (!pollThread->isRunning()) {
        pollThread->setContinuePreviousRun(false);
        pollThread->setConfig(configData);
        pollThread->start();
    }
}

void AnalysisPage::abortThread()
{
    // Switch the buttons so you can't abort twice
    abortButton->setEnabled(false);
    // Try to kill the threads
    if(pollThread!=NULL) {
        pollThread->stop();
        pollThread->wait();
        delete pollThread;
        pollVortexUpdate(NULL);
        cappiDisplay->clearImage();
    }
    delete atcf;
    configDialog->turnOffMembers(false);
    runButton->setEnabled(true);
}


bool AnalysisPage::isRunning()
{
    if (pollThread!=NULL)
        return true;
    else
        return false;
}

void AnalysisPage::saveDisplay()
{
    graph->saveImage();
}

void AnalysisPage::saveLog()
{
    statusLog->saveLogFile();
}

void AnalysisPage::catchLog(const Message& message)
{
    emit log(message);
}

void AnalysisPage::updateCappiInfo(float x, float y, float rmwEstimate, float sMin, float sMax, float vMax,
                                   float userCenterLat, float userCenterLon, float centerLat, float centerLon)
{
    lcdCenterLat->setText(QString().setNum(centerLat,'f', 2));
    lcdCenterLon->setText(QString().setNum(centerLon,'f', 2));
    lcdUserCenterLat->setText(QString().setNum(userCenterLat,'f', 2));
    lcdUserCenterLon->setText(QString().setNum(userCenterLon,'f', 2));
    emit updateMadis(userCenterLat, userCenterLon);
}

void AnalysisPage::updateCappiDisplay(bool hasImage)
{
    if(hasImage) {
        QChar deg(0x00B0);
        QString vel = QString().setNum(cappiDisplay->getMaxApp(),'f', 1);
        QString loc = "at " + QString().setNum(cappiDisplay->getMaxAppHeight(),'f', 1) + " km alt, " +
            QString().setNum(cappiDisplay->getMaxAppDir(),'f', 1) + deg + ", " +
            QString().setNum(cappiDisplay->getMaxAppDist(),'f', 1) + " km range";
        appMaxWind->setText(vel);
        appMaxLabel2->setText(loc);
        QString msg = "Maximum inbound velocity of " + vel + " m/s " + loc + " (";
        vel = QString().setNum(cappiDisplay->getMaxApp()*1.9438445,'f', 1);
        loc = "at " + QString().setNum(cappiDisplay->getMaxAppHeight()*3280.8399,'f', 1) + " feet alt, " +
        QString().setNum(cappiDisplay->getMaxAppDir(),'f', 1) + deg + ", " +
        QString().setNum(cappiDisplay->getMaxAppDist()*0.5399568,'f', 1) + " nm range";
        msg += vel + " kts " + loc + " )";
        emit log(Message(msg,0,this->objectName()));

        vel = QString().setNum(cappiDisplay->getMaxRec(),'f', 1);
        loc = "at " + QString().setNum(cappiDisplay->getMaxRecHeight(),'f', 1) + " km alt, " +
            QString().setNum(cappiDisplay->getMaxRecDir(),'f', 1) + deg + ", " +
            QString().setNum(cappiDisplay->getMaxRecDist(),'f', 1) + " km range";
        recMaxWind->setText(vel);
        recMaxLabel2->setText(loc);
        msg = "Maximum outbound velocity of " + vel + " m/s " + loc + " (";
        vel = QString().setNum(cappiDisplay->getMaxRec()*1.9438445,'f', 1);
        loc = "at " + QString().setNum(cappiDisplay->getMaxRecHeight()*3280.8399,'f', 1) + " feet alt, " +
        QString().setNum(cappiDisplay->getMaxRecDir(),'f', 1) + deg + ", " +
        QString().setNum(cappiDisplay->getMaxRecDist()*0.5399568,'f', 1) + " nm range";
        msg += vel + " kts " + loc + " )";
        emit log(Message(msg,0,this->objectName()));

    }
    else {
        appMaxWind->setText(QString().setNum(0));
        recMaxWind->setText(QString().setNum(0));
        appMaxLabel2->setText(QString());
        recMaxLabel2->setText(QString());
    }

}

void AnalysisPage::autoScroll()
{
    int pos = statusText->verticalScrollBar()->value();
    if(pos == lastMax) {
        lastMax = statusText->verticalScrollBar()->maximum();
        statusText->verticalScrollBar()->setValue(lastMax);
    }
    else
        lastMax = statusText->verticalScrollBar()->maximum();
}

void AnalysisPage::prepareToClose()
{
    //abortThread();
    //Message::toScreen("prepare to Close "+workingDirectory.path());
    QString autoLogName = statusLog->getLogFileName();
    QFile autoLog(workingDirectory.filePath(autoLogName));
    autoLog.remove();

    // autoImage is deleted in the GraphFace destructor

}

bool AnalysisPage::analyticModel()
{

    // Creates a rudimentry dialog for editing or confirming the parameters of
    // the model vortex to be created and sampled by the theoretical radar.

    QString modelFile = configData->getParam(configData->getConfig("radar"),
                                             "dir");
    Configuration *modelConfig = new Configuration(this, modelFile);
    connect(modelConfig, SIGNAL(log(const Message&)),
            this, SLOT(catchLog(const Message&)));
    QDialog *modelDialog = new QDialog;
    modelDialog->setModal(true);
    QPushButton *run = new QPushButton("Create Analytic Model");
    run->setDefault(true);
    QPushButton *cancel = new QPushButton("Cancel");
    ConfigTree *configTreeModel = new ConfigTree(modelDialog, modelConfig);
    connect(configTreeModel, SIGNAL(log(const Message&)),
            this, SLOT(catchLog(const Message&)));

    connect(configTreeModel, SIGNAL(newParam(const QDomElement&,
                                             const QString&, const QString&)),
            modelConfig, SLOT(setParam(const QDomElement&,
                                       const QString&, const QString&)));
    connect(configTreeModel, SIGNAL(addDom(const QDomElement&, const QString&,
                                           const QString&)),
            modelConfig, SLOT(addDom(const QDomElement&,
                                     const QString&, const QString&)));
    connect(configTreeModel, SIGNAL(removeDom(const QDomElement&,
                                              const QString&)),
            modelConfig, SLOT(removeDom(const QDomElement&,const QString&)));

    if(!configTreeModel->read())
        emit log(Message(QString("Error Reading Analytic Storm Configuration"),0,
                         this->objectName()));

    connect(run, SIGNAL(pressed()), modelDialog, SLOT(accept()));
    connect(cancel, SIGNAL(pressed()), modelDialog, SLOT(reject()));

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(run);
    buttons->addWidget(cancel);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(configTreeModel);
    layout->addLayout(buttons);
    modelDialog->setLayout(layout);
    modelDialog->exec();

    if(modelDialog->result()==QDialog::Accepted)
        return true;
    else
        return false;

}

void AnalysisPage::pollVortexUpdate(VortexList* list)
{
    if (list==NULL) {
        currPressure->setText(QString().setNum(0));
        currRMW->setText(QString().setNum(0));
        currDeficit->setText(QString().setNum(0));
        deficitLabel->setText(tr("Pressure Deficit Unavailable"));
        emit vortexListChanged(NULL);
	return;
    }
    //Message::toScreen("Made it too pollVortexUpdate in AnalysisPage");
    if(list->count() > 0) {

        // Find the outermost vtd mean wind coefficient that is not equal to -999
        float maxRadius = list->last().getMaxValidRadius();
        currPressure->setText(QString().setNum((int)list->last().getPressure()));

        if(list->last().getAveRMW()==-999.0)
            currRMW->setText(QString().setNum(0));
        else
            currRMW->setText(QString().setNum(list->last().getAveRMW()));

        currDeficit->setText(QString().setNum(list->last().getPressureDeficit(), 'f', 1));
        deficitLabel->setText(tr("Pressure Deficit From ")+QString().setNum(maxRadius)+tr(" km (mb):"));
        emit vortexListChanged(list);
    }
    else {
        currPressure->setText(QString().setNum(0));
        currRMW->setText(QString().setNum(0));
        currDeficit->setText(QString().setNum(0));
        deficitLabel->setText(tr("Pressure Deficit Unavailable"));
        emit vortexListChanged(NULL);
    }
}

void AnalysisPage::popUpStatus()
{

    QDialog *popUp = new QDialog();
    popUp->resize(statusText->width(), 600);
    popUp->window()->setWindowTitle(tr("Status Log"));
    textPopUp = new QTextEdit();
    textPopUp->setDocument(statusText->document());
    textPopUp->setReadOnly(true);
    textPopUp->resize(statusText->width(), 600);
    textPopUp->verticalScrollBar()->setValue(statusText->verticalScrollBar()->maximum());

    QHBoxLayout *popUpLayout = new QHBoxLayout;
    popUpLayout->addWidget(textPopUp);
    popUp->setLayout(popUpLayout);

    textPopUp = statusText;
    popUp->show();

}

void AnalysisPage::updateStatusLog(const QString& entry)
{

    QTextCursor cursor(statusText->document());
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(entry);
    cursor.endEditBlock();
    statusText->verticalScrollBar()->setValue(statusText->verticalScrollBar()->maximum());
}

/*
void AnalysisPage::changeStopLight(StopLightColor color, const QString message)
{
  emit newStopLightColor(color, message);
}

void AnalysisPage::changeStormSignal(StormSignalStatus status, 
         const QString message)
{
  emit newStormSignalStatus(status, message);
}
*/

void AnalysisPage::openConfigDialog()
{
    configDialog->checkConfig();
    configDialog->show();
}
