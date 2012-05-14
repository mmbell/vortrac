/*
 * PressureList.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef PRESSURELIST_H
#define PRESSURELIST_H

#include <QList>
#include <QString>

#include "Pressure/PressureData.h"

class PressureList : public QList<PressureData>
{

public:
    PressureList(QString prsFilePath=QString());
    virtual ~PressureList();
    bool saveXML();
    bool restore();
    void setFilePath(QString prsFilePath);
private:
    QString _filePath;
    void createDomPressureDataEntry(const PressureData &newData);
};

#endif
