/*
 * PressureList.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "PressureList.h"

PressureList::PressureList(QString prsFilePath) : QList<PressureData>()
{
    _filePath = prsFilePath;
}
PressureList::~PressureList()
{

}
void PressureList::setFilePath(QString prsFilePath)
{
    _filePath=prsFilePath;
}

bool PressureList::saveXML()
{
    return false;
}
bool PressureList::restore()
{
    return false;
}
