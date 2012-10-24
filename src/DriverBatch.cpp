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

DriverBatch::DriverBatch()
{
}

DriverBatch::~DriverBatch()
{
}

bool DriverBatch::initialize(const QDomElement &configuration)
{
//    // Parse the XML configuration file
//    if (!parseXMLconfig(configuration)) return false;

//    // Validate the configuration
//    if (!validateXMLconfig()) return false;

    return true;
}

bool DriverBatch::run()
{
    return true;
}

bool DriverBatch::finalize()
{
    return true;
}
