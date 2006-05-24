/*
 * VortexList.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef VORTEXLIST_H
#define VORTEXLIST_H

#include "VortexData.h"
#include <QList>
#include "Configuration.h"
#include <QString>
#include <QObject>

class VortexList : public QList<VortexData>
{

public:
     VortexList(const QString &newFileName = QString());
     virtual ~VortexList();
     
     bool save();
     bool open();
     void setFileName(const QString &newFileName);
     QString getFileName() { return fileName; }
     void timeSort();
     
     void append(const VortexData &value);

private:
     Configuration *config;
     QString fileName;
     
     void createDomVortexDataEntry(const VortexData& newData);
     


};

#endif
