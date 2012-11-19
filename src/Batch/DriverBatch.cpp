/*
 * DriverBatch.cpp
 * VORTRAC
 *
 * Created by Annette Foerster in 2012
 * Copyright 2005 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 *
 */

#include "DriverBatch.h"
#include <QTimer>

DriverBatch::DriverBatch(QWidget *parent, const QString &fileName)
    : QWidget(parent)
{
    this->setObjectName("Analysis Batchmode Driver");
    xmlfile = fileName;
    statusLog = new Log();

    connect(this, SIGNAL(log(const Message&)),statusLog, SLOT(catchLog(const Message&)));

    // Create a new configuration instance
    configData = new Configuration;
    connect(configData, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));

    statusText = new QTextEdit;

    statusLog->catchLog(Message("VORTRAC Status Log for "+QDateTime::currentDateTime().toUTC().toString()+ " UTC"));

    pollThread = NULL;
    atcf = NULL;
    madis = NULL;
    fetchremote = NULL;

}

DriverBatch::~DriverBatch()
{
    delete configData;
    delete statusLog;
    delete statusText;
    delete pollThread;
}

bool DriverBatch::initialize()
{
    // Parse the XML configuration file
    if (!parseXMLconfig()) return false;

    return true;
}

bool DriverBatch::run()
{
    std::cout << "running ...\n";
    pollThread = new workThread();
    return true;
}

bool DriverBatch::finalize()
{
    std::cout << "finalizing ...\n";
    QTimer::singleShot(0, this->parentWidget(), SLOT(close()));
    return true;
}

bool DriverBatch::parseXMLconfig()
{
    std::cout << "Parsing configuration file ...\n";
    this->loadFile(xmlfile);
    return true;
}

bool DriverBatch::validateXMLconfig()
{   //Not neccessary. Validation is already done within Configuration::read!
    return true;
}

void DriverBatch::saveLog()
{
    statusLog->saveLogFile();
}

void DriverBatch::catchLog(const Message& message)
{
    emit log(message);
}

bool DriverBatch::loadFile(const QString &fileName)
{
    //This is AnalysisPage::loadFile, but without configDialog->read();

    std::cout << "Loading xml file ...\n";
    // Load up a configuration file in the ConfigurationDialog
    if (!configData->read(fileName)) {
        emit log(Message(QString("Couldn't load configuration file"),0,this->objectName()));
        return false;
    }

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
