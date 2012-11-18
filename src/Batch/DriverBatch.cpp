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

DriverBatch::DriverBatch(QWidget *parent, QDomDocument& xmlfile)
    : QWidget(parent)
{
    domDoc = &xmlfile;
}

DriverBatch::~DriverBatch()
{
}

bool DriverBatch::initialize()
{


    // Parse the XML configuration file
    if (!parseXMLconfig()) return false;

    // Validate the 3D specific parameters
    if (!validateXMLconfig()) return false;

    return true;
}

bool DriverBatch::run()
{
    std::cout << "running ...\n";
//    pollThread = new workThread();
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
    return true;
}

bool DriverBatch::validateXMLconfig()
{
    std::cout << "Validating configuration file...\n";
    return true;
}
